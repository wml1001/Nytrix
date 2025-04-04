#pragma once
//PuppetManager.h
#include<WinSock2.h>
#include<string>
#include<WS2tcpip.h>
#include<thread>

#include "ServerCommon.h"
#include "PuppetConnectDispatcher.h"
// ����ÿ������Ŀ��ܶ�����
void PuppetHandle(int clientID, SOCKET puppetSocket, ServerManager* serverManager, ClientConnectionManager& instance);
void PuppetDnsHandle(int clientID, SOCKET puppetSocket, sockaddr_in puppetAddr, int puppetLen, ServerManager* serverManager, ClientConnectionManager& instance);
// �ȴ����ܶ�����
void HandlePuppet(SOCKET listenSocket_puppet, ServerManager* serverManager, ClientConnectionManager& instance,std::string protocol,std::string port);
void HandleDnsPuppet(SOCKET listenSocket_puppet, ServerManager* serverManager, ClientConnectionManager& instance, std::string protocol, std::string port);

void puppetListenerCreate(std::string command, ServerManager* serverManager, ClientConnectionManager& instance);