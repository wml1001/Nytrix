#pragma once
//PayloadGeneration.h
#include<string>
#include<vector>

namespace FileSave {
    bool attackFileSave(std::string name, std::string payload, std::string ip, std::string port,LPWSTR filePath,LPWSTR fileName);
    bool payloadSave(std::string name,std::string payload,std::string ip,std::string port);
}