//Cmd.cpp
#include "Cmd.h"
#include <algorithm>
#include <cctype>

std::string trim_non_printable(const std::string& input) {
    if (input.empty()) return input;

    // ��ĩβ��ǰ�ҵ���һ���ɴ�ӡ�ַ�
    auto it = input.rbegin();
    while (it != input.rend() && !std::isprint(static_cast<unsigned char>(*it))) {
        ++it;
    }

    // ������Ч���ֵĳ���
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

    // ���������߳�
    std::thread recvThread([authenticationManager]() {
        const int CMD_LEN_BUF = 4096;
        std::vector<char> buffer(CMD_LEN_BUF);
        while (authenticationManager->getGRunning()) {  // ���ݱ�־����ѭ��
            int iRet = recv(authenticationManager->getScoket(), buffer.data(), buffer.size() - 1, 0);
            //std::cout << "get!"<< authenticationManager->getScoket();
            if (iRet > 0) {
                buffer[iRet] = '\0';
                std::string receivedData(buffer.data(), iRet);

                //�ж��ǿ��ܶ˼��뻹���������
                if (receivedData.find("puppet:") != std::string::npos) {
                    authenticationManager->setPuppetInfo(receivedData);
                    std::cout << "���յ�pp";
                }
                else
                {
                    std::cout << "���յ�dd";
                    authenticationManager->setReceivedData(trim_non_printable(receivedData));
                }
            }
            else if (iRet == 0) {  // ����������ر�����
                authenticationManager->setGRunning(false);
            }
            else {
                int error = WSAGetLastError();
                std::cout << "error" << error;
                if (error == WSAECONNRESET || error == WSAEINTR) {
                    // ���ӱ����û������رգ�����Ϊ����
                    authenticationManager->setGRunning(false);
                }
                else {
                    //here
                    authenticationManager->setGRunning(false);
                }
            }
        }
        });

    recvThread.detach(); // ���̺߳�̨����
}