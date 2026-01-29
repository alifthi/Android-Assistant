#include "llama.h"
#include <string>

// Forwarding types
typedef struct llama_model llama_model_forwarded;
typedef struct llama_vocab llama_vocab_forwarded;
typedef struct llama_context llama_context_fowarded;
typedef struct llama_sampler llama_sampler_forwarded;
typedef int32_t llama_token;
enum ggml_log_level;

// llama_inference type contains any variable related to llamacpp
typedef struct 
{
    llama_model_forwarded* model;
    llama_context_fowarded* ctx;
    const llama_vocab_forwarded* vocab;
    llama_token* prompt_tokens;
    llama_sampler_forwarded* smplr;
    int n_prompt;
} llama_inference;

// load llama.cpp backed
void load_backend();
