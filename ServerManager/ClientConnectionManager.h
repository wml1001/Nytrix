#pragma once
//ClientConnectionManager.h
#include<mutex>
#include<map>

#include "UserManager.h"

class ClientConnectionManager
{
public:

	static ClientConnectionManager& instance();

	int getActiveClientID();

	void setActiveClientID(int clientID);

	std::shared_ptr<UserManager> getClient(int clientID);

	std::map<int, std::shared_ptr<UserManager>> getAllClient();

	void addClient(std::shared_ptr<UserManager> client);

	int removeClient(int clientID);

	int getClientIDCounter();

	void setClientIDCounter(int clientIDCounter);

	void increase();

	bool getGRunning();

	void setGRunning(bool g_running);

	std::string getPassCode();

	void setPassCode(std::string passCode);

private:

	ClientConnectionManager() = default;

	std::mutex socketMutex;

	int activeClientID = -1;  // 当前交互的客户端 ID (-1 表示未选定)

	int clientIDCounter = 1; // 客户端连接计数器

	std::atomic<bool> g_running{true};  // 全局运行控制标志

	std::string passCode = "@shiyue"; //服务器密码设置

	std::map<int, std::shared_ptr<UserManager>> clientSockets;

};