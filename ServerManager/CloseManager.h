#pragma once
//CloseManager.h

#include "ClientConnectionManager.h"
#include "ServerManager.h"
#include "ServerCommon.h"

// 添加全局关闭函数
void ShutdownServer(ServerManager* serverManager, ClientConnectionManager& instance);