#include "UserManager.h"

// 构造函数
UserManager::UserManager(
    int userId,
    std::string userName,
    std::string userPassWord,
    std::string ip,
    SOCKET socket,
    bool isControl_,
    std::string protocol) {

    this->userId = userId;
    this->userName = userName;
    this->userPassWord;
    this->ip = ip;
    this->socket = socket;
    this->isControl_ = isControl_;
    this->protocol_ = protocol;
}
UserManager::UserManager()
    : userId(0), userName("weiwei~"), userPassWord("@shiyue"), ip(""), socket(INVALID_SOCKET), isControl_(true),protocol_("tcp")
{
}

// 析构函数
UserManager::~UserManager()
{
}

// Getter 方法实现
int UserManager::getUserId() const
{
    return userId;
}

std::string UserManager::getUserName() const
{
    return userName;
}

std::string UserManager::getUserPassword() const
{
    return userPassWord;
}

std::string UserManager::getIp() const
{
    return ip;
}

SOCKET UserManager::getSocket() const
{
    return socket;
}

bool UserManager::isControl() const
{
    return isControl_;
}
std::string UserManager::getProtocol() const {

    return protocol_;
}

sockaddr_in UserManager::getClientSockAddr() const
{
    return clientSockAddr_;
}

int UserManager::getClientSockAddrLen() const
{
    return clientSockAddrLen_;
}

std::string UserManager::getPort() const
{
    return port_;
}

// Setter 方法实现
void UserManager::setUserId(int id)
{
    userId = id;
}

void UserManager::setUserName(const std::string& name)
{
    userName = name;
}

void UserManager::setUserPassword(const std::string& password)
{
    userPassWord = password;
}

void UserManager::setIp(const std::string& ipAddress)
{
    ip = ipAddress;
}

void UserManager::setSocket(SOCKET sock)
{
    socket = sock;
}

void UserManager::setControl(bool control)
{
    isControl_ = control;
}

void UserManager::setProtocol(std::string protocol) {

    protocol_ = protocol;
}

void UserManager::setClientSockAddr(sockaddr_in clientSockAddr)
{
    clientSockAddr_ = clientSockAddr;
}

void UserManager::setClientSockAddrLen(int clientSockAddrLen)
{
    clientSockAddrLen_ = clientSockAddrLen;
}

void UserManager::setPort(std::string port)
{
    port_ = port;
}
