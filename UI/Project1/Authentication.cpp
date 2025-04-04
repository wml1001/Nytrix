// Authentication.cpp
#include "Authentication.h"

namespace Authentication {

    char userNameBuff[bufSize] = { 0 };
    char userPassCodeBuff[bufSize] = { 0 };
    char ip[bufSize] = { 0 };
    char port[bufSize] = { 0 };

    bool authentication(AuthenticationManager* authenticationManager) {
        
        ImGui::Begin("Authentication");

        // 计算输入框宽度，使其对齐
        float labelWidth = 80.0f;  // 统一标签宽度，避免错位
        float inputWidth = ImGui::GetContentRegionAvail().x - labelWidth;
        float windowWidth = ImGui::GetContentRegionAvail().x; // 获取窗口可用宽度

        // 用户名
        ImGui::Text("User Name:"); ImGui::SameLine(labelWidth);
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##username", userNameBuff, IM_ARRAYSIZE(userNameBuff));

        // 密码（隐藏输入）
        ImGui::Text("Pass Code:"); ImGui::SameLine(labelWidth);
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##passcode", userPassCodeBuff, IM_ARRAYSIZE(userPassCodeBuff), ImGuiInputTextFlags_Password);

        // IP
        ImGui::Text("IP:"); ImGui::SameLine(labelWidth);
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##ip", ip, IM_ARRAYSIZE(ip));

        // 端口
        ImGui::Text("Port:"); ImGui::SameLine(labelWidth);
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##port", port, IM_ARRAYSIZE(port));

        // 添加一个空行
        ImGui::Spacing();
        ImGui::Spacing();

        // 计算按钮宽度
        float buttonWidth = 120.0f;
        float centerPosX = (windowWidth - buttonWidth) * 0.5f; // 计算按钮居中位置
        ImGui::SetCursorPosX(centerPosX); // 让按钮水平居中

        // Connect 按钮
        bool resultpass = false;
        if (ImGui::Button("Connect", ImVec2(buttonWidth, 0))) {

            authenticationManager->setUserName(userNameBuff);
            authenticationManager->setPassCode(userPassCodeBuff);
            authenticationManager->setIP(ip);
            authenticationManager->setPort(port);

            resultpass =  authenticationManager->authenticationImplementation();
        }

        ImGui::End();
        return resultpass;
    }
}
