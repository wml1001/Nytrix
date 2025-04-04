
#include "CommandDispatcher.h"

void StartCommandDispatcher(ServerManager* serverManager, ClientConnectionManager& instance) {

    while (instance.getGRunning()) {

        serverManager->setGRunning(instance.getGRunning());

        serverManager->waitForCommands();

        if (serverManager->hasCommands())
        {

            for (auto& client : instance.getAllClient()) {

                if (client.second->isControl()) {

                    std::string toClient = stringCombination(serverManager->getCmdList(true), "\n", serverManager->getCmdList(false));
                    if (SOCKET_ERROR == send(client.second->getSocket(), toClient.c_str(), toClient.size(), 0)) {

                        safePrint("·¢ËÍÊ§°Ü: ", WSAGetLastError());
                    }
                }
            }

            serverManager->CmdListEmpty();
            serverManager->ReturnListClear();
        }
    }
}