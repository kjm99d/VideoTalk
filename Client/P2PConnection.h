#pragma once
#include "CandidateUtils.h"

#include <winsock2.h>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

class P2PConnection {
public:
    P2PConnection();
    ~P2PConnection();

    int GetPort();

    bool Connect(const Candidate& peerCandidate);
    void StartSend();
    void StartReceive();

private:
    SOCKET udpSocket;
    sockaddr_in peerAddr;
    std::thread sendThread;
    std::thread recvThread;
    bool isConnected;

    void SendLoop();
    void ReceiveLoop();
};
