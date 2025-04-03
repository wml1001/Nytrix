#include<Windows.h>
#include<iostream>
#include <vector>
#include<mutex>

#include "ServerManager.h"

ServerManager::ServerManager()
{
}

ServerManager::~ServerManager()
{
}

std::string ServerManager::getCmdList(bool controlCmd) {

	std::string cmd = "empty list...";
	
	if (controlCmd) {

		if (this->controlCmdListServer.empty()) {

			std::cout << "命令队列为空!!!" << std::endl;

			return cmd;
		}

		std::queue<std::string> tmp_queue = this->controlCmdListServer;
		cmd = "";
		while (!tmp_queue.empty()) {

			cmd.append(tmp_queue.front());
			tmp_queue.pop();
		}
	}
	else
	{
		if (this->puppetReturnServer.empty()) {

			std::cout << "命令队列为空!!!" << std::endl;

			return cmd;
		}

		std::queue<std::string> tmp_queue = this->puppetReturnServer;
		cmd = "";
		while (!tmp_queue.empty()) {

			cmd.append(tmp_queue.front());
			tmp_queue.pop();
		}
	}

	return cmd;
}

int ServerManager::setCmdList(std::string cmd, bool controlCmd) {

	if (controlCmd) {

		this->CmdListEmpty();//清楚命令队列的数据，避免和上一条命令粘连

		const size_t max_cmd_length = 4096;

		if (max_cmd_length < cmd.length())
		{

			std::cout << "输入命令过长!!!" << std::endl;

			return -1;

		}

		{
			std::lock_guard<std::mutex> lock(this->controlCmdListServerMutex);
			this->controlCmdListServer.push(cmd);
		}
	}
	else
	{
		this->ReturnListClear();//清楚返回队列的数据，避免和上一条返回数据粘连

		{
			std::lock_guard<std::mutex> lock(this->puppetReturnServerMutex);

			this->puppetReturnServer.push(cmd);
		}
	}

	this->cv.notify_one();

	return 0;
}

bool ServerManager::CmdListIsEmpty() {

	return this->controlCmdListServer.empty();
}

bool ServerManager::ReturnListIsEmpty() {

	return this->puppetReturnServer.empty();
}

int ServerManager::CmdListLength() {

	std::string cmd_return = this->getCmdList(true);

	return cmd_return.length();
}

int ServerManager::ReturnListLength() {

	std::string cmd_return = this->getCmdList(false);

	return cmd_return.length();
}

int ServerManager::ReturnListClear() {

	while (!this->puppetReturnServer.empty()) {

		this->puppetReturnServer.pop();
	}

	if (this->puppetReturnServer.empty()) {
		return 0;
	}

	return -1;
}

int ServerManager::CmdListEmpty() {
	
	while (!this->controlCmdListServer.empty()) {

		this->controlCmdListServer.pop();
	}

	if (this->controlCmdListServer.empty()) {
		return 0;
	}

	return -1;
}

void ServerManager::notifyNewCommand() {

	this->cv.notify_one();
}

void ServerManager::waitForCommands() {

	std::unique_lock<std::mutex> lock(this->puppetReturnServerMutex);
	
	cv.wait(lock, [this] {

		return !this->controlCmdListServer.empty() || !this->puppetReturnServer.empty() || ! this->getGRunning();
		});
}

bool ServerManager::hasCommands() {

	return !this->puppetReturnServer.empty();
}

bool ServerManager::getGRunning() {

	return this->g_running;
}

void ServerManager::setGRunning(bool g_running) {

	this->g_running = g_running;
}