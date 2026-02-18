#include <iostream>
#include <sstream>
#include <string.h>
#include "inference.h"
#include "prompt.h"
#include "states.h"
#include "config.h"

struct Args {
    std::string model_path;
    std::string prompt;
    int n_predict = 128;
    int n_ctx = 2048;
    int n_threads = 4;
};

static void print_usage(const char * prog) {
    std::cerr
        << "Usage: " << prog << " -m <model.gguf>\n"
        << "Options:\n"
        << "  -m, --model       Path to GGUF model (required)\n";
}

static std::string read_stdin_all() {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    return ss.str();
}

static bool parse_args(int argc, char ** argv, Args & out) {
    bool force_prompt_from_args = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-m" || a == "--model") {
            if (i + 1 >= argc) return false;
            out.model_path = argv[++i];
        }
    }

    return !out.model_path.empty();
}


int main(int argc, char ** argv){
    Args args;
    int res;

    llama_inference inference;
    state_type state;
    char *user_prompt = NULL;

    memset(&inference, 0, sizeof(llama_inference));
    memset(&state, 0, sizeof(state_type));

    if (!parse_args(argc, argv, args)) {
        print_usage(argv[0]);
        return 1;
    }
    
    init_default_state(&state);
    load_backend();
    
    res = load_model(MODEL_PATH, &inference);
    if(res){
        free(user_prompt);
        free_ptr(&state);
        free_llama_inference(&inference);
        return 1;
    }

    res = get_vocab(&inference);
    if(res){
        free(user_prompt);
        free_ptr(&state);
        free_llama_inference(&inference);
        return 1;
    }

    res = create_ctx(&inference);
    if(res){
        free(user_prompt);
        free_ptr(&state);
        free_llama_inference(&inference);
        return 1;
    }

    res = set_sampler(&inference);
    if(res){
        free(user_prompt);
        free_ptr(&state);
        free_llama_inference(&inference);
        return 1;
    }
    

    while(1){
        user_prompt = get_user_prompt();
        if (user_prompt == NULL)
            break;

        if(strcmp(user_prompt, "exit\n") == 0) {
            free(user_prompt);
            user_prompt = NULL;
            break;
        }
        state.messages = extend_messages(state.messages, "<|im_start|>user\n");
        state.messages = extend_messages(state.messages, user_prompt);
        state.messages = extend_messages(state.messages, "<|im_end|><|im_start|>assistant\n\n");

        int res = allocate_prompt(&inference, &state);
        if(res){
            free(user_prompt);
            free_ptr(&state);
            free_llama_inference(&inference);
            return 1;
        }

        res = run_inference(&inference, &state);
        if(res){
            free(user_prompt);
            free_ptr(&state);
            free_llama_inference(&inference);
            return 1;
        }
        state.messages = extend_messages(state.messages, state.assistant_response);
        state.kv_applied_chars = strlen(state.messages);
        state.messages = extend_messages(state.messages, "<|im_end|>");

        free(user_prompt);
        user_prompt = NULL;
    }

    free(user_prompt);
    free_ptr(&state);
    free_llama_inference(&inference);
    
    return 0;
}
