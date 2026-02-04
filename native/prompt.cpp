#include <stdlib.h>
#include <sys/types.h>
#include "prompt.h"
#include <iostream>

/*
    * To get user prompt from stdin
*/
char* get_user_prompt(){
    char *line = NULL;
    size_t len = 0;
    std::cout << "\033[1;34m->\033[0m ";
    std::cout.flush();
    if (getline(&line, &len, stdin) == -1) {
        free(line);
        return NULL;
    }

    return line;
}
