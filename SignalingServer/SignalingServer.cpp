#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 5000

struct PeerInfo {
    SOCKET socket;
    std::string iceInfo;
};

std::vector<PeerInfo> peers;
std::mutex peerMutex;

void handleClient(SOCKET clientSock) 
{
    char buffer[1024] = { 0 };

    int len = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
    if (len <= 0) 
    {
        closesocket(clientSock);
        return;
    }

    std::string iceData(buffer, len);
    std::cout << "[+] ICE 정보 수신: " << iceData << std::endl;

    {
        std::lock_guard<std::mutex> lock(peerMutex);
        peers.push_back({ clientSock, iceData });
    }

    while (true) 
    {
        std::lock_guard<std::mutex> lock(peerMutex);
        if (peers.size() == 2) {
            PeerInfo& me = (peers[0].socket == clientSock) ? peers[0] : peers[1];
            PeerInfo& other = (peers[0].socket == clientSock) ? peers[1] : peers[0];

            send(clientSock, other.iceInfo.c_str(), (int)other.iceInfo.size(), 0);
            std::cout << "[+] 상대 ICE 정보 전송 완료\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    closesocket(clientSock);
}

int main() 
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        std::cerr << "WSAStartup 실패" << std::endl;
        return 1;
    }

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock == INVALID_SOCKET) 
    {
        std::cerr << "socket 생성 실패" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) 
    {
        std::cerr << "bind 실패" << std::endl;
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    if (listen(serverSock, 2) == SOCKET_ERROR) 
    {
        std::cerr << "listen 실패" << std::endl;
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    std::cout << "📡 시그널링 서버 실행 중 (Windows, PORT " << SERVER_PORT << ")...\n";

    while (true) 
    {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientLen);
        if (clientSock != INVALID_SOCKET) 
        {
            std::cout << "[+] 클라이언트 접속됨\n";
            std::thread(handleClient, clientSock).detach();
        }
    }

    closesocket(serverSock);

    WSACleanup();

    return 0;
}
