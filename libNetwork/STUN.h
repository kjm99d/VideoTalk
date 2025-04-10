#pragma once


int Request_STUN(char* pPublicIP, int nLenPublicIP, int* nPublicPort, char* pRemoteIP, int nLenRemoteIP, int nRemotePort);

int DomainToSockAddr(const char* domain, unsigned short port, char* pRemoteIP, int nLenRemoteIP);