#ifdef _DLL
#define DLL_EXPORT __declspec(dllexport) 
#else
#define DLL_EXPORT __declspec(dllimport) 
#endif

#include <windows.h>



// ������ ������ ������� Public IP ������ ȹ���Ѵ�.
DLL_EXPORT int ResolveDomainToSockAddr(const char* domain, unsigned short port, char * pRemoteIP, int nLenRemoteIP);

DLL_EXPORT int GetLocalNetwork(OUT char* pLocalIP, int nLenLocalIP);

// STUN ������ ��û�Ͽ� ������� ���� IP/PORT ������ ȹ���Ѵ�.
DLL_EXPORT int GetPublicNetwork(OUT char* pPublicIP, int nLenPublicIP, OUT int* nPublicPort, char* pRemoteIP, int nLenRemoteIP, int nRemotePort);