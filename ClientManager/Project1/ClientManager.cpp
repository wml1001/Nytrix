#include<iostream>
#include<Windows.h>

#include "ClientManager.h"

ClientManager::ClientManager()
{
}

ClientManager::~ClientManager()
{
}

std::string ClientManager::getCmdList() {

	if (this->cmdList.empty()) {

		std::cout << "命令队列为空!!!" << std::endl;

		return "";
	}

	std::string cmd = this->cmdList.front();

	this->cmdList.pop();

	return cmd;
}

int ClientManager::setCmdList(std::string cmd) {

	const size_t max_cmd_length = 4096;

	if (max_cmd_length < cmd.length())
	{

		std::cout << "输入命令过长!!!" << std::endl;

		return -1;

	}

	this->cmdList.push(cmd);

	return 0;
}
