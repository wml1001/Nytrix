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

	int activeClientID = -1;  // ��ǰ�����Ŀͻ��� ID (-1 ��ʾδѡ��)

	int clientIDCounter = 1; // �ͻ������Ӽ�����

	std::atomic<bool> g_running{true};  // ȫ�����п��Ʊ�־

	std::string passCode = "@shiyue"; //��������������

	std::map<int, std::shared_ptr<UserManager>> clientSockets;

};