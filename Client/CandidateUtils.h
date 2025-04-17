#pragma once

#include <string>
#include <vector>
#include <sstream>

typedef struct _Candidate {
    std::string foundation;
    int componentID;
    std::string transport;
    uint32_t priority;
    std::string ip;
    uint16_t port;
    std::string type;
} Candidate;

std::string SerializeCandidate(const Candidate& cand);
Candidate DeserializeCandidate(const std::string& str);

std::string SerializeCandidateList(const std::vector<Candidate>& candidates);
std::vector<Candidate> DeserializeCandidateList(const std::string& data);
