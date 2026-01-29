#include "config.h"

typedef struct {
    char* messages;                // Message contains user, system, assistant and tool messages
    char assistant_response[MAX_ASSISTANT_RESPONSE]; // Stores assistant response
    char* search_query;            // Stores the search query
    char* shell_command;           // Stores the shell command to be executed
    char* stdout;                  // Stores the standard output of the executed command
    char* stderr;                  // Stores the standard error of the executed command
    int exit_code;                 // Stores the exit code of the executed command
} state_type;
