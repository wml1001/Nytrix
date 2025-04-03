
#include "NetworkUtilities.h"

// 创建监听 Socket
SOCKET CreateListenSocket(const char* host, USHORT port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup 初始化失败!!!" << std::endl;
        return INVALID_SOCKET;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket 创建失败!!!" << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    int optval = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &serverAddr.sin_addr) <= 0) {
        std::cerr << "无效的 IP 地址: " << host << std::endl;
        return INVALID_SOCKET;
    }

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "绑定失败!!!" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "监听失败!!!" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    std::cout << "服务器已启动, 监听端口: " << port << "..." << std::endl;
    return listenSocket;
}

SOCKET CreateDnsListenSocket(const char* host, USHORT port) {

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::cout << "服务器已启动, 监听端口: " << port << "..." << std::endl;
    return sock;
}