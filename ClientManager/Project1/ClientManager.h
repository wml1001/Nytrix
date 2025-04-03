#pragma once
#include<iostream>
#include<queue>
#include<string>

class ClientManager
{
public:
	ClientManager();
	~ClientManager();

	std::string getCmdList();

	int setCmdList(std::string cmd);
private:
	std::queue<std::string> cmdList;
};
