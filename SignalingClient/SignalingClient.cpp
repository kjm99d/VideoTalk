#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <sstream>
#include <functional>
#include <vector>

#include "Candidate.h"

#pragma comment(lib, "libNetwork.lib")
#include "libNetwork.h"

#pragma comment(lib, "ws2_32.lib")

#include "P2PConnection.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000

Candidate ParseCandidate(const std::string& candidateStr);
std::string GenerateCandidate(const std::string& transport,   // "udp" or "tcp"
                            const std::string& ip,          // ��: "203.0.113.1"
                            uint16_t port,                  // ��: 54321
                            const std::string& type         // "host", "srflx", "relay"
);

int main() 
{
    

    // ==================================================================================================== //  
    // �������� ����Ͽ� STUN ������ Remote IP ������ �����Ѵ�.
    char szRemoteAddr[32] = { 0, };
    const int nRemotePort = 19302;
    ResolveDomainToSockAddr("stun.l.google.com", nRemotePort, szRemoteAddr, 32);
    
    
    // ==================================================================================================== //  
    // 1. STUN ������ ���� P2P ������ ���� Peer �� Public IP/PORT ������ ȹ���Ѵ�.
    char szPublicIP[32] = { 0, };
    int nPublicPort = 0;
    GetPublicNetwork(szPublicIP, 32, &nPublicPort, szRemoteAddr, 32, nRemotePort);

    
    // ==================================================================================================== // 
    // 2. Signaling ������ ICE Candidate ������ �����Ѵ�.
    
    
    
    
    // 2-2. �ñ׳θ� ������ ICE ���� ����
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup ����" << std::endl;
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // �ñ׳θ� ������ TCP �� �����Ѵ�.
    if (sock == INVALID_SOCKET) {
        std::cerr << "���� ���� ����" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "���� ���� ����" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }


    P2PConnection conn;
    // 2-1. STUN �������� ���� ������ ������� ICE Candidate ����
    // std::string candidate = GenerateCandidate("udp", szPublicIP, nPublicPort, "srflx");
    std::string candidate = GenerateCandidate("udp", "192.168.0.9", conn.GetPort(), "host");

    // ���� Peer�� ICE ���� ����
    send(sock, candidate.c_str(), static_cast<int>(candidate.size()), 0);


    // ==================================================================================================== // 
    // 3. ICE Candidate ������ ȹ���Ѵ�.

    // 3-1. ��� ICE ���� ����
    char buffer[2048] = { 0 };
    int recvLen = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (recvLen > 0) 
    {
        std::string peerIce(buffer, recvLen);
        std::cout << "\n[+] ��� ICE ���� ���ŵ�:\n" << peerIce << std::endl;
        
        // ==================================================================================================== // 
        // 4. ��� Peer �� ICE ������ ������� �Ľ��Ѵ�.
        Candidate peerCandidate = ParseCandidate(peerIce);

        // ==================================================================================================== // 
        // 5. ��� Peer �� ICE ������ ������� P2P ���� �õ��� �Ѵ�.
        //P2PConnection conn;
        conn.Connect(peerCandidate);


        // �ʱⰪ ���� ��Ŷ ����
        // Roll Conflict ������ ���� tieBrocker ���� ( ���Ŀ� ������ �� )


        // ==================================================================================================== // 
        // 6. ����̽� ��� ���� ������ ���� ������ P2P ���� ��뿡�� ��/�����Ѵ�.

        // 7. ���α׷� ���� ����
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    else
    {
        std::cerr << "��� ICE ���� ���� ����" << std::endl;
    }


    closesocket(sock);
    WSACleanup();

    return 0;

}

std::string GenerateCandidate(const std::string& transport, const std::string& ip, uint16_t port, const std::string& type)
{

    // �켱���� ���� ���
    const int TYPE_PREF_SRFLX = 100;
    const int TYPE_PREF_HOST = 126;
    const int TYPE_PREF_RELAY = 0;

    // �켱���� ��� ����
    std::function<uint32_t(int, int, int)> calculatePriority =
        [](int typePref, int localPref, int componentID) {
        return (typePref << 24) + (localPref << 8) + (256 - componentID);
        };


    // foundation�� ������ ������ ó���ϰų� ������ ���
    std::string foundation = "123456789";
    int componentID = 1; // �Ϲ������� 1: RTP
    int typePref;

    if (type == "host")      typePref = TYPE_PREF_HOST;
    else if (type == "srflx") typePref = TYPE_PREF_SRFLX;
    else if (type == "relay") typePref = TYPE_PREF_RELAY;
    else                      typePref = 0; // unknown type

    uint32_t priority = calculatePriority(typePref, 65535, 1);

    std::ostringstream oss;
    oss << "candidate:" << foundation << " "
        << componentID << " "
        << transport << " "
        << priority << " "
        << ip << " "
        << port << " "
        << "typ " << type;

    return oss.str();
}


Candidate ParseCandidate(const std::string& candidateStr) {
    std::istringstream iss(candidateStr);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token) 
    {
        tokens.push_back(token);
    }

    if (tokens.size() < 8 || tokens[0].substr(0, 10) != "candidate:") 
    {
        throw std::runtime_error("Invalid candidate string: " + candidateStr);
    }

    Candidate cand;
    cand.foundation = tokens[0].substr(10);  // remove "candidate:" prefix
    cand.componentID = std::stoi(tokens[1]);
    cand.transport = tokens[2];
    cand.priority = static_cast<uint32_t>(std::stoul(tokens[3]));
    cand.ip = tokens[4];
    cand.port = static_cast<uint16_t>(std::stoi(tokens[5]));

    // typ �� ���������� "typ" �ڿ� ��ġ�ϹǷ� ��ġ �����
    if (tokens[6] != "typ") {
        throw std::runtime_error("Invalid candidate format: missing 'typ'");
    }

    cand.type = tokens[7];

    return cand;
}