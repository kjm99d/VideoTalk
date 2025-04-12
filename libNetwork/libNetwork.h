#ifdef _DLL
#define DLL_EXPORT __declspec(dllexport) 
#else
#define DLL_EXPORT __declspec(dllimport) 
#endif

#include <windows.h>



// 도메인 정보를 기반으로 Public IP 정보를 획득한다.
DLL_EXPORT int ResolveDomainToSockAddr(const char* domain, unsigned short port, char * pRemoteIP, int nLenRemoteIP);

DLL_EXPORT int GetLocalNetwork(OUT char* pLocalIP, int nLenLocalIP);

// STUN 서버에 요청하여 사용자의 공인 IP/PORT 정보를 획득한다.
DLL_EXPORT int GetPublicNetwork(OUT char* pPublicIP, int nLenPublicIP, OUT int* nPublicPort, char* pRemoteIP, int nLenRemoteIP, int nRemotePort);