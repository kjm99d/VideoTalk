#define _WINSOCK_DEPRECATED_NO_WARNINGS   // inet_ntoa 등 비권장 함수 warning 무시
#include <winsock2.h>     // WSAStartup, SOCKET, sockaddr_in, inet_ntoa 등
#include <ws2tcpip.h>     // getaddrinfo, addrinfo, sockaddr_in, etc.
#include <iostream>       // (optional) std::cout, std::cerr 용
#include <cstring>        // strcmp, strncpy_s 등 문자열 함수

#pragma comment(lib, "ws2_32.lib")  // Winsock2 관련 기능

#include "libNetwork.h"
#include "STUN.h"




int ResolveDomainToSockAddr(const char* domain, unsigned short port, char * pRemoteIP, int nLenRemoteIP)
{
	return DomainToSockAddr(domain, port, pRemoteIP, nLenRemoteIP);
}

int GetLocalNetwork(OUT char* pLocalIP, int nLenLocalIP)
{
    if (pLocalIP == nullptr || nLenLocalIP <= 0)
        return -1;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return -2;

    char hostname[256] = { 0, };
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) 
    {
        WSACleanup();
        return -3;
    }

    addrinfo hints{};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) 
    {
        WSACleanup();
        return -4;
    }

    bool found = false;
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) 
    {
        sockaddr_in* sockaddr_ipv4 = reinterpret_cast<sockaddr_in*>(ptr->ai_addr);
        const char* ipStr = inet_ntoa(sockaddr_ipv4->sin_addr);

        if (ipStr && strcmp(ipStr, "127.0.0.1") != 0) 
        {  // 루프백 제외
            strncpy_s(pLocalIP, nLenLocalIP, ipStr, _TRUNCATE);
            found = true;
            break;
        }
    }

    freeaddrinfo(result);
    WSACleanup();

    return found ? 0 : -5;
}

int GetPublicNetwork(OUT char* pPublicIP, int nLenPublicIP, OUT int * nPublicPort, char * pRemoteIP, int nLenRemoteIP, int nRemotePort)
{
	BOOL bRet = ERROR_BUFFER_ALL_ZEROS;

	if (NULL != pPublicIP && NULL != nPublicPort)
	{
		bRet = Request_STUN(pPublicIP, nLenPublicIP, nPublicPort, pRemoteIP, nLenRemoteIP, nRemotePort);
	}

	return bRet;
}