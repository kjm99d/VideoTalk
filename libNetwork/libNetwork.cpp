#include "libNetwork.h"
#include "STUN.h"

int ResolveDomainToSockAddr(const char* domain, unsigned short port, char * pRemoteIP, int nLenRemoteIP)
{
	return DomainToSockAddr(domain, port, pRemoteIP, nLenRemoteIP);
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