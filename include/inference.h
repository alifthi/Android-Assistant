#include "llama.h"
#include "states.h"
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

// function to load model
int load_model(const char* path, llama_inference* inference);

// Get vocab
int get_vocab(llama_inference* inference);

// Allocate memory to prompt
int allocate_prompt(llama_inference* inference, state_type* state);

// Create a llama context
int create_ctx(llama_inference* inference);

// Check if context needs recreation
int needs_ctx_recreation(llama_inference* inference);

// Set sampler
int set_sampler(llama_inference* inference);

// Run inference
int run_inference(llama_inference* inference, state_type* state);

// Unallocate memory 
int free_llama_inference(llama_inference* inference);

// silenses ggml logs
void silent_log_callback(enum ggml_log_level level, const char * text, void * user_data);
