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

		std::cout << "命令队列为空!!!" << std::endl;

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

		std::cout << "输入命令过长!!!" << std::endl;

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
    CloseHandle(hWritePipe); // 关闭父进程的写端

    char buffer[4096];
    DWORD bytesRead;
    bool processExited = false;

    while (!processExited) {
        // 检查进程是否已退出
        DWORD exitCode;
        if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
            break;
        }
        processExited = (exitCode != STILL_ACTIVE);

        // 检查管道数据
        DWORD bytesAvail = 0;
        if (!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &bytesAvail, NULL)) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                break; // 管道已关闭
            }
        }

        if (bytesAvail > 0) {
            if (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                cmd_output.append(buffer, bytesRead);
            }
        }
        else if (!processExited) {
            Sleep(50); // 避免CPU忙等待
        }
    }

    // 读取剩余数据
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        cmd_output.append(buffer, bytesRead);
    }

    WaitForSingleObject(pi.hProcess, INFINITE); // 确保进程完全退出

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    return cmd_output;
}