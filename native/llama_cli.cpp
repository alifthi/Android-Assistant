#include <iostream>
#include <sstream>
#include "inference.h"
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
        << "Usage: " << prog << " -m <model.gguf> [-p <prompt>] [--] [prompt words...]\n"
        << "Options:\n"
        << "  -m, --model       Path to GGUF model (required)\n"
        << "  -p, --prompt      Prompt text (optional; otherwise stdin)\n"
        << "  -n, --n-predict   Number of tokens to generate (default: 128)\n"
        << "  -c, --ctx         Context size (default: 2048)\n"
        << "  -t, --threads     Number of threads (default: 4)\n";
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
        } else if (a == "-p" || a == "--prompt") {
            if (i + 1 >= argc) return false;
            out.prompt = argv[++i];
        } else if (a == "-n" || a == "--n-predict") {
            if (i + 1 >= argc) return false;
            out.n_predict = std::atoi(argv[++i]);
        } else if (a == "-c" || a == "--ctx") {
            if (i + 1 >= argc) return false;
            out.n_ctx = std::atoi(argv[++i]);
        } else if (a == "-t" || a == "--threads") {
            if (i + 1 >= argc) return false;
            out.n_threads = std::atoi(argv[++i]);
        } else if (a == "--") {
            force_prompt_from_args = true;
            for (int j = i + 1; j < argc; ++j) {
                if (!out.prompt.empty()) out.prompt += " ";
                out.prompt += argv[j];
            }
            break;
        } else if (!a.empty() && a[0] == '-') {
            return false;
        } else {
            if (!out.prompt.empty()) out.prompt += " ";
            out.prompt += a;
        }
    }

    if (out.prompt.empty() && !force_prompt_from_args) {
        out.prompt = read_stdin_all();
        if (!out.prompt.empty() && out.prompt.back() == '\n') {
            out.prompt.pop_back();
        }
    }

    return !out.model_path.empty();
}


int main(int argc, char ** argv){
    Args args;
    int res;

    llama_inference inference;
    state_type state;
    char *user_prompt;

    memset(&inference, 0, sizeof(llama_inference));
    memset(&state, 0, sizeof(state_type));

    if (!parse_args(argc, argv, args)) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (args.prompt.empty()) {
        std::cerr << "Empty prompt. Provide -p or stdin.\n";
        return 1;
    }

    load_backend();
    
    
    res = load_model(MODEL_PATH, &inference);
}