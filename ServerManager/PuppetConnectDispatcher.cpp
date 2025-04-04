//PuppetConnectDispatcher.cpp
#include "PuppetConnectDispatcher.h"

void sendPuppetInfo(ClientConnectionManager& clientConnectionManager) {

    int count = 0;
    std::string toClient = "";
    std::map<int, std::shared_ptr<UserManager>> clients = clientConnectionManager.getAllClient();
    for (auto& client : clients) {

        if (client.second->isControl()) {
            for (auto& puppet : clients)
            {
                if (!puppet.second->isControl()) {
                    
                    count++;
                    toClient += stringCombination("puppet:", puppet.second->getUserId(), ":", puppet.second->getIp(),":");
                }
            }
            if (count > 0) {
                if (SOCKET_ERROR == send(client.second->getSocket(), toClient.c_str(), toClient.size(), 0)) {

                    safePrint("发送失败: ", WSAGetLastError());
                }
            }
            else {

                toClient = stringCombination("puppet:");//控制傀儡端断开连接时客户Ui刷新显示

                if (SOCKET_ERROR == send(client.second->getSocket(), toClient.c_str(), toClient.size(), 0)) {

                    safePrint("发送失败: ", WSAGetLastError());
                }
            }
        }
        toClient = "";
        count = 0;
    }
}