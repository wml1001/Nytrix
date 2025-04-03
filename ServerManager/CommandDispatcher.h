#pragma once
//CommandDispatcher.h

#include "ClientConnectionManager.h"
#include "ServerManager.h"
#include "ServerCommon.h"

void StartCommandDispatcher(ServerManager* serverManager, ClientConnectionManager& instance);