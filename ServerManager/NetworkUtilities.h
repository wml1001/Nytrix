#pragma once
// NetworkUtilities.h
#include<winsock2.h>
#include<WS2tcpip.h>
#include<Windows.h>
#include<iostream>

// �������� Socket
SOCKET CreateListenSocket(const char* host, USHORT port);

//����dns socket
SOCKET CreateDnsListenSocket(const char* host, USHORT port);