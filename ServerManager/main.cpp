#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include<sstream>

#pragma comment(lib, "ws2_32.lib")

#include "NetworkUtilities.h"
#include "PuppetManager.h"
#include "ControlManager.h"
#include "CommandDispatcher.h"
#include "CloseManager.h"
#include "resource.h"

void print_usage(const char* prog_name) {
    std::cout << "服务端程序 - 使用指南\n\n"
        << "用法:\n"
        << "  " << prog_name << " <控制端IP> <控制端口> <连接密码>\n"
        << "  " << prog_name << " -h | --help\n\n"
        << "参数说明:\n"
        << "  <控制端IP> \t服务端监听的IP地址（IPv4格式，例：127.0.0.1）\n"
        << "  <控制端口> \t控制端监听的TCP端口（1-65535，例：8088）\n"
        << "  <连接密码> \t服务端认证密码（包含特殊字符时建议用引号包裹）\n\n"
        << "示例:\n"
        << "  " << prog_name << " 192.168.1.100 8088 \"S3cret#123\"\n"
        << "  " << prog_name << " -h\n\n"
        << "注意事项:\n"
        << "  • 所有参数为必填项，请按顺序严格填写\n"
        << "  • 端口冲突会导致程序启动失败\n"
        << "  • 密码验证失败将无法建立连接\n"
        << "  • 本机测试可使用 127.0.0.1 作为回环地址\n";
}

int main(int argc, char* argv[]) {

    //if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
    //    print_usage(argv[0]);
    //    return 0;
    //}

    //// 参数数量校验
    //if (argc != 4) {
    //    std::cerr << "\033[31m错误：参数数量不正确\033[0m\n"
    //        << "• 需要参数：3 个\n"
    //        << "• 收到参数：" << (argc - 1) << " 个\n"
    //        << "请执行 '" << argv[0] << " -h' 查看完整帮助文档\n";
    //    return -1;
    //}

    //char* host = argv[1];
    //char* port_client = argv[2];
    //char* passCode = argv[3];

    const char* host = "127.0.0.1";
    const char* port_client = "8088";
    const char* port_pupet = "8081";
    std::string passCode = "@shiyue"; 

    //safePrint("创建 ServerManager 对象");
    ServerManager* serverManager = new ServerManager();

    //初始化客户端连接实例
    ClientConnectionManager& instance = ClientConnectionManager::instance();
    instance.setPassCode(passCode);

    SOCKET listenSocket_client = CreateListenSocket(host, static_cast<USHORT>(std::stoi(port_client)));
    //SOCKET listenSocket_puppet = CreateListenSocket(host, static_cast<USHORT>(std::stoi(port_pupet)));
    
    //单独创建一个线程用于与控制端的交互
    std::thread clientThread(HandleClient, listenSocket_client, serverManager,std::ref(instance));

    //单独创建一个线程用于与傀儡端进行交互
    //std::thread puppetThread(HandlePuppet, listenSocket_puppet, serverManager,std::ref(instance));

    //单独创建一个命令派发线程
    std::thread dispatcher(StartCommandDispatcher, serverManager, std::ref(instance));

    clientThread.join();
    //puppetThread.join();
    dispatcher.join();

    closesocket(listenSocket_client);
    //closesocket(listenSocket_puppet);
    WSACleanup();

    ShutdownServer(serverManager, std::ref(instance));

    delete serverManager;

    return 0;
}
