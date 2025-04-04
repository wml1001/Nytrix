//AuthenticationManager.cpp
#include<vector>
#include<mutex>

#include "AuthenticationManager.h"

AuthenticationManager::AuthenticationManager(SOCKET mysocket,
	std::string userName,
	std::string passCode,
	std::string ip,
	std::string port,
	bool authenticationPass,
    bool g_running,
    std::string g_receivedData)
{
	this->mysocket = mysocket;
	this->userName = userName;
	this->passCode = passCode;
	this->ip = ip;
	this->port = port;
	this->authenticationPass = authenticationPass;
    this->g_running = g_running;
    this->g_receivedData = g_receivedData;
}

AuthenticationManager::~AuthenticationManager()
{
}

std::string AuthenticationManager::getUserName() {
	return this->userName;
}
std::string AuthenticationManager::getPassCode() {
	return this->passCode;
}
std::string AuthenticationManager::getIP() {
	return this->ip;
}
std::string AuthenticationManager::getPort() {
	return this->port;
}
SOCKET AuthenticationManager::getScoket() {
    std::lock_guard<std::mutex> lock(this->socketMutex);
	return this->mysocket;
}
bool AuthenticationManager::getAuthenticationPass() {
	return this->authenticationPass;
}
bool AuthenticationManager::getGRunning() {
    return this->g_running.load();
}
std::string AuthenticationManager::getReceivedData() {
    std::lock_guard<std::mutex> lock(this->g_recvMutex);
    return this->g_receivedData;
}
std::string AuthenticationManager::getpuppetInfo() {
    return this->puppetInfo;
}

void AuthenticationManager::setUserName(std::string userName) {
	this->userName = userName;
}
void AuthenticationManager::setPassCode(std::string passCode) {
	this->passCode = passCode;
}
void AuthenticationManager::setIP(std::string ip) {
	this->ip = ip;
}
void AuthenticationManager::setPort(std::string port) {
	this->port = port;
}
void AuthenticationManager::setSocket(SOCKET mysocket) {
	this->mysocket = mysocket;
}

void AuthenticationManager::setAuthenticationPass(bool authenticationPass) {
	this->authenticationPass = authenticationPass;
}

void AuthenticationManager::setGRunning(bool g_running) {
    this->g_running.store(g_running);
}
void AuthenticationManager::setReceivedData(std::string receivedData) {
    std::lock_guard<std::mutex> lock(this->g_recvMutex);
    this->g_receivedData += "\n";
    this->g_receivedData += receivedData;
}
void AuthenticationManager::setPuppetInfo(std::string puppetInfo) {
    this->puppetInfo = puppetInfo;
}

void AuthenticationManager::clear() {
    this->g_receivedData = "";
}

bool AuthenticationManager::authenticationImplementation() {

    USHORT usPort;
    int CMD_LEN_BUF = 4096;
    try {
        usPort = static_cast<USHORT>(std::stoi(this->getPort()));
    }
    catch (...) {
        std::cerr << "Invalid port number" << std::endl;
        return FALSE;
    }

    SOCKET hSock = INVALID_SOCKET;

    // 初始化Winsock
    WSADATA stData;
    if (WSAStartup(MAKEWORD(2, 2), &stData) != 0) {
        return FALSE;
    }

    // 创建套接字
    hSock = socket(AF_INET, SOCK_STREAM, 0);
    if (hSock == INVALID_SOCKET) {
        WSACleanup();
        return FALSE;
    }

    // 设置服务器地址
    SOCKADDR_IN stSockAddr = { 0 };
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(usPort);
    if (inet_pton(AF_INET, this->getIP().c_str(), &stSockAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address format" << std::endl;
        closesocket(hSock);
        WSACleanup();
        return FALSE;
    }

    // 连接重试逻辑（2次，间隔2秒）
    bool connected = false;
    for (int retry = 0; retry < 2; ++retry) {
        if (connect(hSock, (SOCKADDR*)&stSockAddr, sizeof(stSockAddr)) == 0) {
            connected = true;
            break;
        }
        std::cerr << "Connection attempt " << (retry + 1) << " failed" << std::endl;
        Sleep(1000);
    }

    if (!connected) {
        closesocket(hSock);
        WSACleanup();
        return FALSE;
    }


    //处理用户的认证信息
    std::string cmd_buff = "";
    cmd_buff = this->getUserName();
    cmd_buff.append(":");
    cmd_buff.append(this->getPassCode());

    if (send(hSock, cmd_buff.c_str(), cmd_buff.size(), 0) == SOCKET_ERROR) {
        return false;
    }
    std::vector<char> buffer(CMD_LEN_BUF);
    int iRet = recv(hSock, buffer.data(), buffer.size() - 1, 0);

    if (iRet > 0) {
        buffer[iRet] = '\0';
        std::string receivedData(buffer.data(), iRet);

        if (receivedData.find("pass!!!") != std::string::npos) {
            this->setSocket(hSock);
            return true;
        }
    }
    else {
        int error = WSAGetLastError();
        return false;
    }
}