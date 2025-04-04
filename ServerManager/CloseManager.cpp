
#include "CloseManager.h"

void ShutdownServer(ServerManager* serverManager, ClientConnectionManager& instance) {

    instance.setGRunning(false);

    // 关闭所有连接
    auto clients = instance.getAllClient();
    for (auto& client : clients) {
        closesocket(client.second->getSocket());
    }

    safePrint("服务器正在关闭...");
}