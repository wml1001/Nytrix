#pragma once
#include<iostream>
#include<queue>
#include<string>

class PuppetManager
{
public:
	PuppetManager();
	~PuppetManager();

	std::string getCmdList();

	int setCmdList(std::string cmd);

	std::string exec();

private:
	std::queue<std::string> cmdList;
};
