#include<iostream>
#include<WinSock2.h>
#include <ws2tcpip.h>
#include<Windows.h>
#include<vector>
#include<thread>
#include<mutex>

#include "ClientManager.h"

#pragma comment(lib,"ws2_32.lib")

#define CMD_LEN_BUF 4096

std::mutex g_coutMutex; // 用于同步标准输出
std::atomic<bool> g_running(true);  // 全局运行控制标志

BOOL StartConnect(const char* pcszIP, const char* usPort1) {
    std::cout << "Creating ClientManager..." << std::endl;

    USHORT usPort;
    try {
        usPort = static_cast<USHORT>(std::stoi(usPort1));
    }
    catch (...) {
        std::cerr << "Invalid port number" << std::endl;
        return FALSE;
    }

    ClientManager* clientManager = new ClientManager();
    SOCKET hSock = INVALID_SOCKET;

    // 初始化Winsock
    WSADATA stData;
    if (WSAStartup(MAKEWORD(2, 2), &stData) != 0) {
        delete clientManager;
        return FALSE;
    }

    // 创建套接字
    hSock = socket(AF_INET, SOCK_STREAM, 0);
    if (hSock == INVALID_SOCKET) {
        delete clientManager;
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
        delete clientManager;
        WSACleanup();
        return FALSE;
    }

    // 连接重试逻辑（5次，间隔5秒）
    bool connected = false;
    for (int retry = 0; retry < 5; ++retry) {
        if (connect(hSock, (SOCKADDR*)&stSockAddr, sizeof(stSockAddr)) == 0) {
            connected = true;
            break;
        }
        std::cerr << "Connection attempt " << (retry + 1) << " failed" << std::endl;
        Sleep(5000);
    }

    if (!connected) {
        closesocket(hSock);
        delete clientManager;
        WSACleanup();
        return FALSE;
    }

    std::cout << "Connected to server!" << std::endl;

    //处理用户的认证信息

    while (true) {

        std::string username = "";
        std::string passcode = "";
        std::string cmd_buff = "";

        {
            std::lock_guard<std::mutex> lock(g_coutMutex);
            std::cout << "welcome to Nytrix !!!" << std::endl;
            std::cout << "username: " << std::flush;
            std::getline(std::cin, username);
            std::cout << "passcode: " << std::flush;
            std::getline(std::cin, passcode);
        }

        cmd_buff.append(username);
        cmd_buff.append(":");
        cmd_buff.append(passcode);

        if (send(hSock, cmd_buff.c_str(), cmd_buff.size(), 0) == SOCKET_ERROR) {
            std::lock_guard<std::mutex> lock(g_coutMutex);
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        }
        std::vector<char> buffer(CMD_LEN_BUF);
        int iRet = recv(hSock, buffer.data(), buffer.size() - 1, 0);

        if (iRet > 0) {
            buffer[iRet] = '\0';
            std::string receivedData(buffer.data(), iRet);

            if (receivedData.find("pass!!!") != std::string::npos ) {
                break;
            }
        }
        else {
            int error = WSAGetLastError();
            std::lock_guard<std::mutex> lock(g_coutMutex);
            std::cerr << "\nrecv failed: " << error << std::endl;
            g_running = false;
            break;
        }
    }
    
    // 创建接收线程
    std::thread recvThread([hSock]() {
        std::vector<char> buffer(CMD_LEN_BUF);
        while (g_running.load()) {  // 根据标志控制循环
            int iRet = recv(hSock, buffer.data(), buffer.size() - 1, 0);

            if (iRet > 0) {
                buffer[iRet] = '\0';
                std::string receivedData(buffer.data(), iRet);
                {
                    std::lock_guard<std::mutex> lock(g_coutMutex);
                    std::cout << "\r\033[K" << receivedData << std::endl; // 清除当前行输出结果
                    std::cout << "$ " << std::flush;               // 强制刷新提示符
                }
            }
            else if (iRet == 0) {  // 服务端正常关闭连接
                g_running = false;
                break;
            }
            else {
                int error = WSAGetLastError();
                if (error == WSAECONNRESET || error == WSAEINTR) {
                    // 连接被重置或主动关闭，不视为错误
                    g_running = false;
                    break;
                }
                else {
                    std::lock_guard<std::mutex> lock(g_coutMutex);
                    std::cerr << "\nrecv failed: " << error << std::endl;
                    g_running = false;
                    break;
                }
            }
        }
        });

    // 主线程处理用户输入
    {
        std::lock_guard<std::mutex> lock(g_coutMutex);
        std::cout << "$ " << std::flush;  // 初始提示符
    }

    while (g_running.load()) {
        std::string cmd_buff = "";
        std::getline(std::cin, cmd_buff);  // 阻塞等待输入

        // 过滤空输入（用户只按回车）
        if (cmd_buff.empty()) {
            std::lock_guard<std::mutex> lock(g_coutMutex);
            std::cout << "$ " << std::flush;  // 重新绘制提示符
            continue;
        }

        if (cmd_buff == "exit") {
            g_running = false;  // 通知接收线程退出
            break;
        }

        if (send(hSock, cmd_buff.c_str(), cmd_buff.size(), 0) == SOCKET_ERROR) {
            std::lock_guard<std::mutex> lock(g_coutMutex);
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            g_running = false;
            break;
        }
    }

    // 清理资源
    shutdown(hSock, SD_BOTH);
    closesocket(hSock);

    if (recvThread.joinable()) {
        recvThread.join();  // 等待接收线程结束
    }

    WSACleanup();
    delete clientManager;

    return TRUE;
}

int main(int argc, char* argv[]) {
    std::cout << "控制端启动!!!" << std::endl;

    if (argc != 3) {
        std::cout << "参数错误：需要服务端IP和端口号!" << std::endl;
        return -1;
    }

    StartConnect(argv[1], argv[2]);
    return 0;
}