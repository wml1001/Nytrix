//AuthenticationManager.h
#pragma once

#include<string>
#include<iostream>
#include<WinSock2.h>
#include<Windows.h>
#include <ws2tcpip.h>
#include<vector>
#include<mutex>

#pragma comment(lib,"ws2_32.lib")

class AuthenticationManager
{
public:
	AuthenticationManager(SOCKET mysocket,
	std::string userName,
	std::string passCode,
	std::string ip,
	std::string port,
	bool authenticationPass,
	bool g_running,
	std::string g_receivedData);

	~AuthenticationManager();

	std::string getUserName();
	std::string getPassCode();
	std::string getIP();
	std::string getPort();
	SOCKET getScoket();
	bool getAuthenticationPass();
	bool getGRunning();
	std::string getReceivedData();
	std::string getpuppetInfo();

	void setUserName(std::string userName);
	void setPassCode(std::string passCode);
	void setIP(std::string ip);
	void setPort(std::string port);
	void setSocket(SOCKET mysocket);
	void setAuthenticationPass(bool authenticationPass);
	void setGRunning(bool g_running);
	void setReceivedData(std::string receivedData);
	void setPuppetInfo(std::string puppetInfo);

	bool authenticationImplementation();
	void clear();

private:

	SOCKET mysocket;
	std::mutex g_recvMutex;
	std::mutex socketMutex;
	std::string g_receivedData;
	std::string puppetInfo;

	std::string userName;
	std::string passCode;
	std::string ip;
	std::string port;
	bool authenticationPass;
	std::atomic<bool> g_running;
};