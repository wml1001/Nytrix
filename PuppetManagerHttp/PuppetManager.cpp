#include<iostream>
#include<Windows.h>

#include "PuppetManager.h"

PuppetManager::PuppetManager()
{
}

PuppetManager::~PuppetManager()
{
}

std::string PuppetManager::getCmdList() {

	if (this->cmdList.empty()) {

		std::cout << "�������Ϊ��!!!" << std::endl;

		return "";
	}

	std::string cmd = this->cmdList.front();

	this->cmdList.pop();

	return cmd;
}

int PuppetManager::setCmdList(std::string cmd) {

	const size_t max_cmd_length = 4096;

	if (max_cmd_length < cmd.length())
	{

		std::cout << "�����������!!!" << std::endl;

		return -1;

	}

	this->cmdList.push(cmd);

	return 0;
}

std::string PuppetManager::exec() {
    std::string cmd = "cmd.exe /c " + this->getCmdList();
    std::string cmd_output;

    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hReadPipe, hWritePipe;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
        std::cerr << "CreatePipe failed (" << GetLastError() << ")" << std::endl;
        return "";
    }

    if (!SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0)) {
        std::cerr << "SetHandleInformation failed (" << GetLastError() << ")" << std::endl;
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return "";
    }

    std::wstring wideCmd = std::wstring(cmd.begin(), cmd.end());
    wchar_t* cmdLine = _wcsdup(wideCmd.c_str());

    STARTUPINFO si = { sizeof(STARTUPINFO) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi;
    if (!CreateProcessW(NULL, cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::cerr << "CreateProcess failed (" << GetLastError() << ")" << std::endl;
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        free(cmdLine);
        return "command fail!!!";
    }

    free(cmdLine);
    CloseHandle(hWritePipe); // �رո����̵�д��

    char buffer[4096];
    DWORD bytesRead;
    bool processExited = false;

    while (!processExited) {
        // �������Ƿ����˳�
        DWORD exitCode;
        if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
            break;
        }
        processExited = (exitCode != STILL_ACTIVE);

        // ���ܵ�����
        DWORD bytesAvail = 0;
        if (!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &bytesAvail, NULL)) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                break; // �ܵ��ѹر�
            }
        }

        if (bytesAvail > 0) {
            if (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                cmd_output.append(buffer, bytesRead);
            }
        }
        else if (!processExited) {
            Sleep(50); // ����CPUæ�ȴ�
        }
    }

    // ��ȡʣ������
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        cmd_output.append(buffer, bytesRead);
    }

    WaitForSingleObject(pi.hProcess, INFINITE); // ȷ��������ȫ�˳�

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    return cmd_output;
}