#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <string>
#include <winsock2.h>  // 包含 SOCKET 和 INVALID_SOCKET 的定义

class UserManager
{
public:
    UserManager();
    UserManager(
        int userId,
        std::string userName,
        std::string userPassWord,
        std::string ip,
        SOCKET socket,
        bool isControl_,
        std::string protocol_);
    ~UserManager();

    // Getter 方法
    int getUserId() const;
    std::string getUserName() const;
    std::string getUserPassword() const;
    std::string getIp() const;
    SOCKET getSocket() const;
    bool isControl() const;
    std::string getProtocol() const;
    sockaddr_in getClientSockAddr() const;
    int getClientSockAddrLen() const;
    std::string getPort()const;

    // Setter 方法
    void setUserId(int id);
    void setUserName(const std::string& name);
    void setUserPassword(const std::string& password);
    void setIp(const std::string& ipAddress);
    void setSocket(SOCKET sock);
    void setControl(bool control);
    void setProtocol(std::string protocol_);
    void setClientSockAddr(sockaddr_in clientSockAddr);
    void setClientSockAddrLen(int clientSockAddrLen);
    void setPort(std::string port);

private:
    int userId;
    std::string userName;
    std::string userPassWord;
    std::string ip;
    std::string port_;
    SOCKET socket;
    bool isControl_;
    std::string protocol_;//信道协议
    
    //针对基于udp的传输
    sockaddr_in clientSockAddr_;
    int clientSockAddrLen_;
};

#endif // USER_MANAGER_H