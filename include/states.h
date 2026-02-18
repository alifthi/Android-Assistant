#pragma once

#include "config.h"
#include <stddef.h>

typedef struct {
    char* messages;                                  // Message contains user, system, assistant and tool messages
    char assistant_response[MAX_ASSISTANT_RESPONSE]; // Stores assistant response
    size_t kv_applied_chars;                         // Number of message bytes already reflected in KV cache
} state_type;

char* extend_messages(char* main_string, const char* addition);

void free_ptr(state_type* state);

int init_default_state(state_type* state);
