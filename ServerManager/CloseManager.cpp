
#include "CloseManager.h"

void ShutdownServer(ServerManager* serverManager, ClientConnectionManager& instance) {

    instance.setGRunning(false);

    // �ر���������
    auto clients = instance.getAllClient();
    for (auto& client : clients) {
        closesocket(client.second->getSocket());
    }

    safePrint("���������ڹر�...");
}