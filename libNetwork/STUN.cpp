#include "STUN.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

#define STUN_SERVER "stun.l.google.com"
#define STUN_PORT 19302
#define MAGIC_COOKIE 0x2112A442

void ParseStunResponse(uint8_t* buffer, int len, char* pPublicIP, int nLenPublicIP, int* nPublicPort);

struct StunHeader {
    uint16_t type;
    uint16_t length;
    uint32_t magicCookie;
    uint8_t transactionId[12];
};

int Request_STUN(char* pPublicIP, int nLenPublicIP, int* nPublicPort, char* pRemoteIP, int nLenRemoteIP, int nRemotePort)
{
    srand((unsigned int)time(nullptr));

    // Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup 실패" << std::endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "socket 생성 실패" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(nRemotePort);
    inet_pton(AF_INET, pRemoteIP, &serverAddr.sin_addr); // nslookup stun.l.google.com

    uint8_t buffer[1024] = { 0 };
    StunHeader* req = (StunHeader*)buffer;
    req->type = htons(0x0001);
    req->length = htons(0);
    req->magicCookie = htonl(MAGIC_COOKIE);

    for (int i = 0; i < 12; ++i)
    {
        req->transactionId[i] = rand() % 256;
    }

    sendto(sock, (char*)buffer, sizeof(StunHeader), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

    sockaddr_in from{};
    int fromLen = sizeof(from);
    int recvLen = recvfrom(sock, (char*)buffer, sizeof(buffer), 0, (sockaddr*)&from, &fromLen);

    if (recvLen > 0)
    {
        ParseStunResponse(buffer, recvLen, pPublicIP, nLenPublicIP, nPublicPort);
    }
    else
    {
        std::cerr << "STUN 응답 없음" << std::endl;
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}

int DomainToSockAddr(const char* domain, unsigned short port, char* pRemoteIP, int nLenRemoteIP)
{
    WSADATA wsaData;
    (VOID)WSAStartup(MAKEWORD(2, 2), &wsaData);


    addrinfo hints{};
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_DGRAM;

    addrinfo* res = nullptr;
    int result = getaddrinfo(domain, nullptr, &hints, &res);
    if (result != 0 || res == nullptr) {
        std::cerr << "getaddrinfo failed: " << result << std::endl;
        return false;
    }


    sockaddr_in* resolvedAddr = reinterpret_cast<sockaddr_in*>(res->ai_addr);
    //outAddr.sin_family = AF_INET;
    //outAddr.sin_port = htons(port);
    //outAddr.sin_addr = resolvedAddr->sin_addr;

    inet_ntop(AF_INET, &(resolvedAddr->sin_addr), pRemoteIP, nLenRemoteIP);
    //strcpy_s(pRemoteIP, nLenRemoteIP, resolvedAddr->sin_addr);

    freeaddrinfo(res);

    WSACleanup();

    return true;
}

void ParseStunResponse(uint8_t* buffer, int len, char * pPublicIP, int nLenPublicIP, int * nPublicPort)
{
    if (len < 20) {
        //std::cerr << "STUN 응답이 너무 짧음" << std::endl;
        return;
    }

    int pos = 20;
    while (pos < len)
    {
        uint16_t attrType = ntohs(*(uint16_t*)(buffer + pos));
        uint16_t attrLen = ntohs(*(uint16_t*)(buffer + pos + 2));

        if (attrType == 0x0020)
        {
            uint8_t family = buffer[pos + 5];
            uint16_t xPort = ntohs(*(uint16_t*)(buffer + pos + 6)) ^ (MAGIC_COOKIE >> 16);
            uint32_t xAddr = ntohl(*(uint32_t*)(buffer + pos + 8)) ^ MAGIC_COOKIE;

            in_addr ip;
            ip.S_un.S_addr = htonl(xAddr);
            
            strcpy_s(pPublicIP, nLenPublicIP, inet_ntoa(ip));
            *nPublicPort = xPort;

            //std::cout << "공인 IP: " << inet_ntoa(ip) << std::endl;
            //std::cout << "공인 PORT: " << xPort << std::endl;

            return;
        }

        pos = pos + (4 + attrLen);
        if (attrLen % 4 != 0)
            pos = pos + (4 - (attrLen % 4));
    }

    //std::cerr << "XOR-MAPPED-ADDRESS를 찾을 수 없음" << std::endl;
}