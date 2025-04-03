//ClientConnectionManager.cpp
#include "ClientConnectionManager.h"
#include "UserManager.h"

ClientConnectionManager& ClientConnectionManager::instance() {

	static ClientConnectionManager instance;

	return instance;
}

int ClientConnectionManager::getActiveClientID() {

	return this->activeClientID;
}

void ClientConnectionManager::setActiveClientID(int clientID) {

	std::lock_guard<std::mutex> lock(this->socketMutex);
	this->activeClientID = clientID;
}

std::shared_ptr<UserManager> ClientConnectionManager::getClient(int clientID) {

	std::lock_guard<std::mutex> lock(this->socketMutex);
	auto it = this->clientSockets.find(clientID);

	if (it != this->clientSockets.end()) {

		return it->second;
	}

	return nullptr;
}

std::map<int, std::shared_ptr<UserManager>> ClientConnectionManager::getAllClient() {
	std::lock_guard<std::mutex> lock(this->socketMutex);
	return this->clientSockets;
}
void ClientConnectionManager::addClient(std::shared_ptr<UserManager> client) {
	std::lock_guard<std::mutex> lock(this->socketMutex);
	int clientID = client->getUserId();
	this->clientSockets[clientID] = client;
}

int ClientConnectionManager::removeClient(int clientID) {
	std::lock_guard<std::mutex> lock(this->socketMutex);
	this->clientSockets.erase(clientID);

	if (this->activeClientID == clientID) {

		this->activeClientID = -1;
	}

	return clientID;
}

int ClientConnectionManager::getClientIDCounter() {

	std::lock_guard<std::mutex> lock(this->socketMutex);
	return this->clientIDCounter;
}

void ClientConnectionManager::setClientIDCounter(int clientIDCounter) {

	std::lock_guard<std::mutex> lock(this->socketMutex);
	this->clientIDCounter = clientIDCounter;
}

void ClientConnectionManager::increase() {

	std::lock_guard<std::mutex> lock(this->socketMutex);
	this->clientIDCounter++;
}

bool ClientConnectionManager::getGRunning() {

	return this->g_running.load();
}

void ClientConnectionManager::setGRunning(bool g_running) {

	this->g_running.store(g_running);
}

std::string ClientConnectionManager::getPassCode() {

	return this->passCode;
}

void ClientConnectionManager::setPassCode(std::string passCode) {

	this->passCode = passCode;
}