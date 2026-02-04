#include "inference.h"
#include "llama.h"
#include "states.h"
#include <string.h>
#include <config.h>

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
int load_model(char *path, llama_inference *inference){
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
    inference->n_prompt = -llama_tokenize(inference->vocab, state->messages, strlen(state->messages), NULL, 0, true, true);
    inference->prompt_tokens = static_cast<llama_token*>(
        malloc(inference->n_prompt * sizeof(llama_token))
    );
    if (llama_tokenize(inference->vocab, state->messages, strlen(state->messages),
                       inference->prompt_tokens, inference->n_prompt, true, true) < 0) {
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
    
    struct llama_batch batch = llama_batch_get_one(inference->prompt_tokens, inference->n_prompt);
    size_t prompt_len = strlen(state->messages);  
    size_t max_len = 2048;             
     
    int n_decode = 0;
    llama_token new_token_id;
    state->assistant_response[0] = '\0';
    for (int n_pos = 0; n_pos + batch.n_tokens < inference->n_prompt + max_len; ) {
        if (llama_decode(inference->ctx, batch)) {
            fprintf(stderr, "[Error] Failed to evaluate batch\n");
            return 1;
        }
        n_pos += batch.n_tokens;
        new_token_id = llama_sampler_sample(inference->smplr, inference->ctx, -1);
        if (llama_vocab_is_eog(inference->vocab, new_token_id)) {
            break;
        }
        
        char buf[128];
        int n = llama_token_to_piece(inference->vocab, new_token_id, buf, sizeof(buf), 0, true);
        
        if (n < 0) {
            fprintf(stderr, "[Error] Failed to convert token to piece\n");
            return 1;
        }
        if (prompt_len + n < max_len - 1) {
            strncat(state->assistant_response, buf,n);
            prompt_len += n;
        } else {
            fprintf(stderr, "[Warning] assistant_response buffer full, truncating.\n");
            break;
        }
        printf("\033[0;32m%.*s\033[0m", n, buf);
        fflush(stdout);
        batch = llama_batch_get_one(&new_token_id, 1);   
        n_decode++;
    }
    printf("\n");
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
