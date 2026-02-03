#include "inference.h"
#include "llama.h"
#include "states.h"
#include <string>
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