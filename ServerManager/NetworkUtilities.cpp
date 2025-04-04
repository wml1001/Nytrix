
#include "NetworkUtilities.h"

// �������� Socket
SOCKET CreateListenSocket(const char* host, USHORT port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup ��ʼ��ʧ��!!!" << std::endl;
        return INVALID_SOCKET;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket ����ʧ��!!!" << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    int optval = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &serverAddr.sin_addr) <= 0) {
        std::cerr << "��Ч�� IP ��ַ: " << host << std::endl;
        return INVALID_SOCKET;
    }

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "��ʧ��!!!" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "����ʧ��!!!" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    std::cout << "������������, �����˿�: " << port << "..." << std::endl;
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

    std::cout << "������������, �����˿�: " << port << "..." << std::endl;
    return sock;
}