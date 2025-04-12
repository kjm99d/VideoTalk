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
                            const std::string& ip,          // 예: "203.0.113.1"
                            uint16_t port,                  // 예: 54321
                            const std::string& type         // "host", "srflx", "relay"
);

int main() 
{
    

    // ==================================================================================================== //  
    // 도메인을 사용하여 STUN 서버의 Remote IP 정보를 수집한다.
    char szRemoteAddr[32] = { 0, };
    const int nRemotePort = 19302;
    ResolveDomainToSockAddr("stun.l.google.com", nRemotePort, szRemoteAddr, 32);
    
    
    // ==================================================================================================== //  
    // 1. STUN 서버를 통해 P2P 연결을 위한 Peer 의 Public IP/PORT 정보를 획득한다.
    char szPublicIP[32] = { 0, };
    int nPublicPort = 0;
    GetPublicNetwork(szPublicIP, 32, &nPublicPort, szRemoteAddr, 32, nRemotePort);

    
    // ==================================================================================================== // 
    // 2. Signaling 서버로 ICE Candidate 정보를 전달한다.
    
    
    
    
    // 2-2. 시그널링 서버로 ICE 정보 전달
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup 실패" << std::endl;
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 시그널링 서버는 TCP 로 연결한다.
    if (sock == INVALID_SOCKET) {
        std::cerr << "소켓 생성 실패" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "서버 연결 실패" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }


    P2PConnection conn;
    // 2-1. STUN 서버에서 받은 정보를 기반으로 ICE Candidate 생성
    // std::string candidate = GenerateCandidate("udp", szPublicIP, nPublicPort, "srflx");
    std::string candidate = GenerateCandidate("udp", "192.168.0.9", conn.GetPort(), "host");

    // 현재 Peer의 ICE 정보 전송
    send(sock, candidate.c_str(), static_cast<int>(candidate.size()), 0);


    // ==================================================================================================== // 
    // 3. ICE Candidate 정보를 획득한다.

    // 3-1. 상대 ICE 정보 수신
    char buffer[2048] = { 0 };
    int recvLen = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (recvLen > 0) 
    {
        std::string peerIce(buffer, recvLen);
        std::cout << "\n[+] 상대 ICE 정보 수신됨:\n" << peerIce << std::endl;
        
        // ==================================================================================================== // 
        // 4. 상대 Peer 의 ICE 정보를 기반으로 파싱한다.
        Candidate peerCandidate = ParseCandidate(peerIce);

        // ==================================================================================================== // 
        // 5. 상대 Peer 의 ICE 정보를 기반으로 P2P 연결 시도를 한다.
        //P2PConnection conn;
        conn.Connect(peerCandidate);


        // 초기값 설정 패킷 전송
        // Roll Conflict 방지를 위한 tieBrocker 정보 ( 추후에 개선할 것 )


        // ==================================================================================================== // 
        // 6. 디바이스 장비를 통해 가져온 영상 정보를 P2P 연결 상대에게 송/수신한다.

        // 7. 프로그램 종료 방지
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    else
    {
        std::cerr << "상대 ICE 정보 수신 실패" << std::endl;
    }


    closesocket(sock);
    WSACleanup();

    return 0;

}

std::string GenerateCandidate(const std::string& transport, const std::string& ip, uint16_t port, const std::string& type)
{

    // 우선순위 계산용 상수
    const int TYPE_PREF_SRFLX = 100;
    const int TYPE_PREF_HOST = 126;
    const int TYPE_PREF_RELAY = 0;

    // 우선순위 계산 공식
    std::function<uint32_t(int, int, int)> calculatePriority =
        [](int typePref, int localPref, int componentID) {
        return (typePref << 24) + (localPref << 8) + (256 - componentID);
        };


    // foundation은 간단히 난수로 처리하거나 고정값 사용
    std::string foundation = "123456789";
    int componentID = 1; // 일반적으로 1: RTP
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

    // typ 는 고정적으로 "typ" 뒤에 위치하므로 위치 보장됨
    if (tokens[6] != "typ") {
        throw std::runtime_error("Invalid candidate format: missing 'typ'");
    }

    cand.type = tokens[7];

    return cand;
}