#pragma once
//ServerCommon.h
#include<iostream>
#include<map>
#include<mutex>
#include<atomic>
#include<string>
#include<WinSock2.h>
#include<sstream>

#include "ClientConnectionManager.h"
#include "ServerManager.h"

#define BUFFER_SIZE 4096 //���ڽ����������ݵĻ�������С
inline std::mutex coutMutex; // ����ͬ������̨���

template<typename... Args>
void safePrint(const Args&... args) {
    std::lock_guard<std::mutex> lock(coutMutex);
    ((std::cout << args << " "), ...);
    std::cout << std::endl;
}

template <typename... Args>
std::string stringCombination(const Args&... args) {

    std::ostringstream oss;

    ((oss << args << " "), ...);

    std::string str = oss.str();

    if (!str.empty()) {

        str.pop_back();
    }

    return str;
}

int clientAndPuppetConnectionDeal(ClientConnectionManager& instance, char* ip, SOCKET sock, sockaddr_in clientSockAddr, bool isControl, std::string protocol,std::string port);

