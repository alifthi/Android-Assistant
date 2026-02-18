#include "inference.h"
#include "llama.h"
#include "states.h"
#include <string.h>
#include <config.h>
#include <iostream>
#include <limits.h>

/*
    * loading ggml backend
*/
void load_backend(){
#if defined(__ANDROID__)
    // Avoid scanning root directories on Android; CPU backend is built-in.
#else
    ggml_backend_load_all();
#endif
}

/*
    * To silent ggml logs
*/
void silent_log_callback(enum ggml_log_level level, const char * text, void * user_data) {
    (void)level;
    (void)text;
    (void)user_data;
}

/*
    * Loading model
    * @param path: The path of model.
    * @param inference: inference object.
    * @return: returns 0 if load was succeed
*/
int load_model(const char *path, llama_inference *inference){
    struct llama_model_params model_params = llama_model_default_params();
    llama_log_set(silent_log_callback, NULL);    
    inference->model = llama_model_load_from_file(path, model_params);
    if(inference->model == NULL){
        fprintf(stderr,"[Error] Faild to load model %s\n", path);
        return 1;
    }
    return 0;
}

/*
    * Loading vocab
    * @param inference: inference object.
    * @return: returns 0 if load was succeed
*/
int get_vocab(llama_inference *inference){
    inference->vocab = llama_model_get_vocab(inference->model);
    if(inference->vocab == NULL){
        fprintf(stderr,"[Error] Faild to load vocab\n");
        return 1;
    }
    return 0;
}

/*
    * Initialization of llama contex
    * @param inference: Inference object. 
*/
int create_ctx(llama_inference* inference){
    struct llama_context_params ctx_params = llama_context_default_params();
    ctx_params.no_perf = false;
    ctx_params.n_ctx = N_CTX; 
    ctx_params.n_batch = N_BATCH; 
    ctx_params.n_threads = N_THREADS;

    inference->ctx = llama_init_from_model(inference->model, ctx_params);
    if (inference->ctx == NULL){
        fprintf(stderr, "[Error] Failed initialize contex\n");
        return 1;
    }
    return 0;
}

/*
    * Set sampler
    * @param inferenceL llama_inference object 
*/
int set_sampler(llama_inference* inference){
    struct llama_sampler_chain_params chain_params = llama_sampler_chain_default_params();
    chain_params.no_perf = false;

    inference->smplr = llama_sampler_chain_init(chain_params);
    llama_sampler_chain_add(inference->smplr, llama_sampler_init_greedy());
    return 0;
}

/*
    * allocate prompt_tokens
    * @param inference: inference object.
    * @param state: state object.
    * @return: returns 0 if tokenization was succeed.
*/
int allocate_prompt(llama_inference* inference, state_type* state){
    if (state->messages == NULL) {
        fprintf(stderr, "[Error] Missing conversation state\n");
        return 1;
    }

    const size_t message_len = strlen(state->messages);
    if (state->kv_applied_chars > message_len) {
        fprintf(stderr, "[Error] Invalid KV cache cursor\n");
        return 1;
    }

    const char * delta_prompt = state->messages + state->kv_applied_chars;
    printf("%s", delta_prompt);
    const size_t delta_len = message_len - state->kv_applied_chars;
    if (delta_len > static_cast<size_t>(INT32_MAX)) {
        fprintf(stderr, "[Error] Prompt segment is too large to tokenize\n");
        return 1;
    }

    const bool add_special = state->kv_applied_chars == 0;
    inference->n_prompt = -llama_tokenize(
        inference->vocab,
        delta_prompt,
        static_cast<int32_t>(delta_len),
        NULL,
        0,
        add_special,
        true
    );
    if (inference->n_prompt < 0) {
        fprintf(stderr, "[Error] Failed to size prompt tokenization buffer\n");
        return 1;
    }

    if (inference->prompt_tokens != NULL) {
        free(inference->prompt_tokens);
        inference->prompt_tokens = NULL;
    }

    if (inference->n_prompt == 0) {
        return 0;
    }

    inference->prompt_tokens = static_cast<llama_token*>(
        malloc(inference->n_prompt * sizeof(llama_token))
    );
    if (inference->prompt_tokens == NULL) {
        fprintf(stderr, "[Error] Failed to allocate prompt token buffer\n");
        return 1;
    }

    if (llama_tokenize(
            inference->vocab,
            delta_prompt,
            static_cast<int32_t>(delta_len),
            inference->prompt_tokens,
            inference->n_prompt,
            add_special,
            true
        ) < 0) {
        fprintf(stderr, "[Error] Failed to tokenize the prompt\n");
        return 1;
    }
    return 0;
}

/*
    * Runing inference
    * @param inference: A llama_inference object
*/
int run_inference(llama_inference* inference, state_type* state) {
    std::cout.setf(std::ios::unitbuf);

    const uint32_t n_ctx = llama_n_ctx(inference->ctx);
    llama_memory_t memory = llama_get_memory(inference->ctx);
    const llama_pos pos_max = llama_memory_seq_pos_max(memory, 0);
    const int n_cached = pos_max < 0 ? 0 : static_cast<int>(pos_max + 1);

    if (inference->n_prompt <= 0) {
        std::cerr << "[Error] No new prompt tokens to evaluate.\n";
        return 1;
    }

    if (n_cached + inference->n_prompt >= static_cast<int>(n_ctx)) {
        std::cerr << "[Error] Context window exhausted (cached=" << n_cached
                  << ", new_prompt=" << inference->n_prompt
                  << ", n_ctx=" << n_ctx << ").\n";
        return 1;
    }

    struct llama_batch prompt_batch = llama_batch_get_one(inference->prompt_tokens, inference->n_prompt);
    if (llama_decode(inference->ctx, prompt_batch)) {
        std::cerr << "[Error] Failed to evaluate prompt batch\n";
        return 1;
    }

    size_t max_len = static_cast<size_t>(n_ctx - n_cached - inference->n_prompt);
    if (max_len == 0) {
        std::cerr << "[Error] No room left to generate response tokens.\n";
        return 1;
    }

    state->assistant_response[0] = '\0';
    const size_t resp_cap = sizeof(state->assistant_response) - 1;
    size_t resp_len = 0;
    bool warned_trunc = false;

    for (size_t n_decode = 0; n_decode < max_len; ++n_decode) {
        llama_token new_token_id = llama_sampler_sample(inference->smplr, inference->ctx, -1);

        if (llama_vocab_is_eog(inference->vocab, new_token_id)) {
            break;
        }

        char buf[128];
        int n = llama_token_to_piece(inference->vocab, new_token_id, buf, sizeof(buf), 0, true);
        if (n < 0) {
            std::cerr << "[Error] Failed to convert token to piece\n";
            return 1;
        }
        if (resp_len + static_cast<size_t>(n) <= resp_cap) {
            strncat(state->assistant_response, buf, n);
            resp_len += static_cast<size_t>(n);
        } else if (!warned_trunc) {
            std::cerr << "[Warning] assistant_response buffer full; continuing stream without storing full text.\n";
            warned_trunc = true;
        }
        std::cout << "\033[0;32m";
        std::cout.write(buf, n);
        std::cout << "\033[0m" << std::flush;

        struct llama_batch token_batch = llama_batch_get_one(&new_token_id, 1);
        if (llama_decode(inference->ctx, token_batch)) {
            std::cerr << "[Error] Failed to evaluate generation batch\n";
            return 1;
        }
    }
    std::cout << '\n' << std::flush;
    return 0;
}

/*
    * Unallocates llama_inference object
    * @param inference: A llama_inference object
*/
int free_llama_inference(llama_inference* inference){
    if (inference == NULL) {
        return 0;
    }

    if (inference->model != NULL) {
        llama_model_free(inference->model);
        inference->model = NULL;
    }

    if (inference->smplr != NULL) {
        llama_sampler_free(inference->smplr);
        inference->smplr = NULL;
    }

    if (inference->ctx != NULL) {
        llama_free(inference->ctx);
        inference->ctx = NULL;
    }

    if (inference->prompt_tokens != NULL) {
        free(inference->prompt_tokens);
        inference->prompt_tokens = NULL;
    }
    
    return 0;
}
