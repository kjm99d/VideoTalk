#pragma once
#include <string>

typedef struct _Candidate {
    std::string foundation;
    int componentID;
    std::string transport;
    uint32_t priority;
    std::string ip;
    uint16_t port;
    std::string type;
} Candidate;