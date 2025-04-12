#include "CandidateUtils.h"
#include <stdexcept>

// 단일 Candidate 문자열로 변환
std::string SerializeCandidate(const Candidate& cand) {
    std::ostringstream oss;
    oss << "candidate:" << cand.foundation << " "
        << cand.componentID << " "
        << cand.transport << " "
        << cand.priority << " "
        << cand.ip << " "
        << cand.port << " "
        << "typ " << cand.type;
    return oss.str();
}

// 단일 Candidate 문자열 파싱
Candidate DeserializeCandidate(const std::string& str) {
    std::istringstream iss(str);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() < 8 || tokens[0].substr(0, 10) != "candidate:") {
        throw std::runtime_error("Invalid candidate string: " + str);
    }

    Candidate cand;
    cand.foundation = tokens[0].substr(10);
    cand.componentID = std::stoi(tokens[1]);
    cand.transport = tokens[2];
    cand.priority = static_cast<uint32_t>(std::stoul(tokens[3]));
    cand.ip = tokens[4];
    cand.port = static_cast<uint16_t>(std::stoi(tokens[5]));

    if (tokens[6] != "typ") {
        throw std::runtime_error("Invalid format: missing 'typ'");
    }

    cand.type = tokens[7];
    return cand;
}

// Candidate 리스트 → 문자열
std::string SerializeCandidateList(const std::vector<Candidate>& candidates) {
    std::ostringstream oss;
    for (const auto& cand : candidates) {
        oss << SerializeCandidate(cand) << "\n";
    }
    return oss.str();
}

// 문자열 → Candidate 리스트
std::vector<Candidate> DeserializeCandidateList(const std::string& data) {
    std::vector<Candidate> list;
    std::istringstream iss(data);
    std::string line;

    while (std::getline(iss, line)) {
        if (!line.empty()) {
            list.push_back(DeserializeCandidate(line));
        }
    }

    return list;
}
