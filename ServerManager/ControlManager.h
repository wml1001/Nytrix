#pragma once
//ControlManager.h
#include<WinSock2.h>
#include<string>
#include<WS2tcpip.h>
#include<thread>
#include<sstream>

#include "ServerCommon.h"

void ControlHandle(int clientID, SOCKET clientSocket, ServerManager* serverManager, ClientConnectionManager& instance); 

// 处理控制端输入命令
void HandleClient(SOCKET listenSocket_client, ServerManager* serverManager, ClientConnectionManager& instance);