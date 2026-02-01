#pragma once

#include "config.h"

typedef struct {
    char* messages;                // Message contains user, system, assistant and tool messages
    char assistant_response[MAX_ASSISTANT_RESPONSE]; // Stores assistant response
} state_type;
