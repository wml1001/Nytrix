#pragma once
//PuppetManager.h
#include<WinSock2.h>
#include<string>
#include<WS2tcpip.h>
#include<thread>

#include "ServerCommon.h"
#include "PuppetConnectDispatcher.h"
// 处理每个具体的傀儡端连接
void PuppetHandle(int clientID, SOCKET puppetSocket, ServerManager* serverManager, ClientConnectionManager& instance);
void PuppetDnsHandle(int clientID, SOCKET puppetSocket, sockaddr_in puppetAddr, int puppetLen, ServerManager* serverManager, ClientConnectionManager& instance);
// 等待傀儡端连接
void HandlePuppet(SOCKET listenSocket_puppet, ServerManager* serverManager, ClientConnectionManager& instance,std::string protocol,std::string port);
void HandleDnsPuppet(SOCKET listenSocket_puppet, ServerManager* serverManager, ClientConnectionManager& instance, std::string protocol, std::string port);

void puppetListenerCreate(std::string command, ServerManager* serverManager, ClientConnectionManager& instance);