// Authentication.cpp
#include "Authentication.h"

namespace Authentication {

    char userNameBuff[bufSize] = { 0 };
    char userPassCodeBuff[bufSize] = { 0 };
    char ip[bufSize] = { 0 };
    char port[bufSize] = { 0 };

    bool authentication(AuthenticationManager* authenticationManager) {
        
        ImGui::Begin("Authentication");

        // ����������ȣ�ʹ�����
        float labelWidth = 80.0f;  // ͳһ��ǩ��ȣ������λ
        float inputWidth = ImGui::GetContentRegionAvail().x - labelWidth;
        float windowWidth = ImGui::GetContentRegionAvail().x; // ��ȡ���ڿ��ÿ��

        // �û���
        ImGui::Text("User Name:"); ImGui::SameLine(labelWidth);
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##username", userNameBuff, IM_ARRAYSIZE(userNameBuff));

        // ���루�������룩
        ImGui::Text("Pass Code:"); ImGui::SameLine(labelWidth);
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##passcode", userPassCodeBuff, IM_ARRAYSIZE(userPassCodeBuff), ImGuiInputTextFlags_Password);

        // IP
        ImGui::Text("IP:"); ImGui::SameLine(labelWidth);
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##ip", ip, IM_ARRAYSIZE(ip));

        // �˿�
        ImGui::Text("Port:"); ImGui::SameLine(labelWidth);
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##port", port, IM_ARRAYSIZE(port));

        // ���һ������
        ImGui::Spacing();
        ImGui::Spacing();

        // ���㰴ť���
        float buttonWidth = 120.0f;
        float centerPosX = (windowWidth - buttonWidth) * 0.5f; // ���㰴ť����λ��
        ImGui::SetCursorPosX(centerPosX); // �ð�ťˮƽ����

        // Connect ��ť
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
