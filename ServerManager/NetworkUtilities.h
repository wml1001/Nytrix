#pragma once
// NetworkUtilities.h
#include<winsock2.h>
#include<WS2tcpip.h>
#include<Windows.h>
#include<iostream>

// 创建监听 Socket
SOCKET CreateListenSocket(const char* host, USHORT port);

//创建dns socket
SOCKET CreateDnsListenSocket(const char* host, USHORT port);