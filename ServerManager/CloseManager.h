#pragma once
//CloseManager.h

#include "ClientConnectionManager.h"
#include "ServerManager.h"
#include "ServerCommon.h"

// ���ȫ�ֹرպ���
void ShutdownServer(ServerManager* serverManager, ClientConnectionManager& instance);