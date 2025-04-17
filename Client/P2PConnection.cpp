#include "P2PConnection.h"
#include <iostream>
#include <chrono>
#include <ws2tcpip.h>  // inet_pton

const std::string kPingMessage = "ICE-CONNECT-PING";

P2PConnection::P2PConnection()
    : isConnected(false)
{
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "[x] UDP 소켓 생성 실패\n";
    }

    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = 0; // 랜덤 포트
    localAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        std::cerr << "[x] UDP bind 실패: " << WSAGetLastError() << "\n";
    }
}


P2PConnection::~P2PConnection() 
{
    if (udpSocket != INVALID_SOCKET) 
    {
        closesocket(udpSocket);
    }
}

int P2PConnection::GetPort()
{
    if (udpSocket == INVALID_SOCKET) return -1;

    sockaddr_in addr{};
    int len = sizeof(addr);

    if (getsockname(udpSocket, (sockaddr*)&addr, &len) == SOCKET_ERROR) {
        std::cerr << "[x] getsockname() 실패: " << WSAGetLastError() << "\n";
        return -1;
    }

    return ntohs(addr.sin_port);
}

bool P2PConnection::Connect(const Candidate& peerCandidate) 
{
    peerAddr = {};
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(peerCandidate.port);
    inet_pton(AF_INET, peerCandidate.ip.c_str(), &peerAddr.sin_addr);

    // 연결 확인용 PING 메시지 전송
    sendto(udpSocket, kPingMessage.c_str(), static_cast<int>(kPingMessage.size()), 0,
        (sockaddr*)&peerAddr, sizeof(peerAddr));
    std::cout << "[>] PING 메시지 전송: " << peerCandidate.ip << ":" << peerCandidate.port << "\n";

    char buffer[1024] = { 0 };
    sockaddr_in fromAddr{};
    int fromLen = sizeof(fromAddr);

    
    int recvLen = -1;
    
    recvLen = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&fromAddr, &fromLen);
    if (recvLen == SOCKET_ERROR) {
        int errCode = WSAGetLastError();
        std::cerr << "[recvfrom ERROR] 코드: " << errCode << std::endl;
    }

    if (recvLen > 0 && std::string(buffer, recvLen) == kPingMessage) 
    {
        std::cout << "[✔] P2P 연결 성공! 상대가 PING 응답\n";
        isConnected = true;
        StartSend();
        StartReceive();
        return true;
    }

    std::cerr << "[x] P2P 연결 실패. 상대의 응답 없음\n";
    return false;
}

void P2PConnection::StartSend() 
{
    sendThread = std::thread(&P2PConnection::SendLoop, this);
    sendThread.detach();
}

void P2PConnection::StartReceive() 
{
    recvThread = std::thread(&P2PConnection::ReceiveLoop, this);
    recvThread.detach();
}

void P2PConnection::SendLoop() 
{
    while (isConnected) {
        std::string msg = "Hello from this peer!";
        sendto(udpSocket, msg.c_str(), static_cast<int>(msg.size()), 0,
            (sockaddr*)&peerAddr, sizeof(peerAddr));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void P2PConnection::ReceiveLoop() 
{
    while (isConnected) {
        char buffer[1024] = { 0 };
        sockaddr_in fromAddr{};
        int fromLen = sizeof(fromAddr);

        int recvLen = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
            (sockaddr*)&fromAddr, &fromLen);
        if (recvLen > 0) {
            std::string msg(buffer, recvLen);
            std::cout << "[recv] " << msg << "\n";
        }
    }
}
