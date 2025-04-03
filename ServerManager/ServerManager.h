#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include<iostream>
#include<string>
#include<queue>
#include<mutex>

class ServerManager
{
public:
	ServerManager();
	~ServerManager();

	std::string getCmdList(bool controlCmd= true);

	int setCmdList(std::string cmd, bool controlCmd = true);

	bool CmdListIsEmpty();

	bool ReturnListIsEmpty();

	int CmdListLength();

	int ReturnListLength();

	int ReturnListClear();
	int CmdListEmpty();

	//用户事件驱动而不是轮询等待
	void notifyNewCommand();

	void waitForCommands();

	bool hasCommands();

	bool getGRunning();

	void setGRunning(bool g_running);

private:

	bool g_running = true;

	std::mutex controlCmdListServerMutex;
	std::mutex puppetReturnServerMutex;

	std::condition_variable cv;

	std::queue<std::string> controlCmdListServer;
	std::queue<std::string> puppetReturnServer;
};
#endif // !SERVER_MANAGER_H