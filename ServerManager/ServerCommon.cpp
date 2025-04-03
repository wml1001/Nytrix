
#include "ServerCommon.h"

int clientAndPuppetConnectionDeal(ClientConnectionManager& instance, char* ip, SOCKET sock,sockaddr_in clientSockAddr, bool isControl,std::string protocol,std::string port) {

    int clientID = 0;
    clientID = instance.getClientIDCounter();
    auto client = std::make_shared<UserManager>();
    client->setUserId(clientID);
    client->setIp(ip);
    client->setControl(isControl);
    client->setProtocol(protocol);
    client->setPort(port);

    if (protocol.substr(0, 3) == "dns") {

        client->setSocket(sock);
        client->setClientSockAddr(clientSockAddr);
        client->setClientSockAddrLen(sizeof(clientSockAddr));
    }
    else
    {
        sockaddr_in clientSock;
        memset(&clientSock, 0, sizeof(clientSock));

        client->setSocket(sock);
        client->setClientSockAddr(clientSock);
        client->setClientSockAddrLen(0);
    }

    instance.addClient(client);

    instance.increase();

    return clientID;
}