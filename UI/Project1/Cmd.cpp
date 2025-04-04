//Cmd.cpp
#include "Cmd.h"
#include <algorithm>
#include <cctype>

std::string trim_non_printable(const std::string& input) {
    if (input.empty()) return input;

    // 从末尾向前找到第一个可打印字符
    auto it = input.rbegin();
    while (it != input.rend() && !std::isprint(static_cast<unsigned char>(*it))) {
        ++it;
    }

    // 计算有效部分的长度
    size_t end_pos = input.size() - std::distance(input.rbegin(), it);
    return input.substr(0, end_pos);
}
void ProcessCommand(char* buf, AuthenticationManager* authenticationManager) {

    std::string tmp = buf;
    if (tmp.find("clear") != std::string::npos) {
        authenticationManager->clear();
    }
    else
    {
        if (send(authenticationManager->getScoket(), buf, strlen(buf), 0) == SOCKET_ERROR) {
            authenticationManager->setGRunning(false);
        }
    }
}

void waitMyCmdReturn(AuthenticationManager* authenticationManager) {

    // 创建接收线程
    std::thread recvThread([authenticationManager]() {
        const int CMD_LEN_BUF = 4096;
        std::vector<char> buffer(CMD_LEN_BUF);
        while (authenticationManager->getGRunning()) {  // 根据标志控制循环
            int iRet = recv(authenticationManager->getScoket(), buffer.data(), buffer.size() - 1, 0);
            //std::cout << "get!"<< authenticationManager->getScoket();
            if (iRet > 0) {
                buffer[iRet] = '\0';
                std::string receivedData(buffer.data(), iRet);

                //判断是傀儡端加入还是命令回显
                if (receivedData.find("puppet:") != std::string::npos) {
                    authenticationManager->setPuppetInfo(receivedData);
                    std::cout << "接收到pp";
                }
                else
                {
                    std::cout << "接收到dd";
                    authenticationManager->setReceivedData(trim_non_printable(receivedData));
                }
            }
            else if (iRet == 0) {  // 服务端正常关闭连接
                authenticationManager->setGRunning(false);
            }
            else {
                int error = WSAGetLastError();
                std::cout << "error" << error;
                if (error == WSAECONNRESET || error == WSAEINTR) {
                    // 连接被重置或主动关闭，不视为错误
                    authenticationManager->setGRunning(false);
                }
                else {
                    //here
                    authenticationManager->setGRunning(false);
                }
            }
        }
        });

    recvThread.detach(); // 让线程后台运行
}