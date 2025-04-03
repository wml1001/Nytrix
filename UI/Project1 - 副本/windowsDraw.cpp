#include "windowsDraw.h"
#include "Menu.h"
#include "utils.h"
#include "PayloadGeneration.h"

namespace App {

    void windowsDrawing(AuthenticationManager* authenticationManager){

        std::unordered_map<std::string, bool>&  window_states = getWindowsStates();
        std::vector<ListenerItem>& listenerItems = getListenerItems();
        std::vector<ListenerItem>& selectListenerItem = getSelectListenerItem();
        //����payload��
        if (window_states["windows"]) {

            //����payloadѡ�񴰿�
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Windows Executable", &window_states["windows"])) {

                float labelWidth = 80.0f;
                float inputWidth = ImGui::GetContentRegionAvail().x - labelWidth;
                std::vector<ListenerItem> generateListener;

                if (selectListenerItem.empty())
                {
                    ListenerItem tmpListenerItem = { "Please first create a listener!!!", "null", "0.0.0.1", "8080" };
                    generateListener.push_back(tmpListenerItem);
                }
                else
                {
                    generateListener.push_back(selectListenerItem.back());
                }

                // �� generateListener �����һ��Ԫ�ص� name ���Ը�ֵ�� listenerBuf
                char listenerBuf[BUFSIZ] = { 0 };
                if (!generateListener.empty()) {
                    strncpy_s(listenerBuf, generateListener.back().name.c_str(), BUFSIZ - 1);
                    listenerBuf[BUFSIZ - 1] = '\0'; // ȷ���ַ����Կ��ַ���β
                    generateListener.pop_back();
                }

                // ͳһ�ؼ���ʽ����
                static const float buttonWidth = 80.0f;

                // ���������ò���
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Listener:");
                ImGui::SameLine(labelWidth);

                // ʹ���������ʾ��������Ϣ��ֻ��ģʽ��
                ImGui::SetNextItemWidth(inputWidth - buttonWidth);
                ImGui::InputText("##listener", listenerBuf, IM_ARRAYSIZE(listenerBuf),
                    ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);

                // ѡ��ť������򱣳�ͬһ��
                ImGui::SameLine();
                if (ImGui::Button("Choose", ImVec2(buttonWidth, 0))) {
                    window_states["listenerWindow"] = true;
                }

                // �������ѡ�񲿷�
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Output:");
                ImGui::SameLine(labelWidth);

                static const char* outputTypes[] = { "EXE", "DLL" };
                static int currentType = 0;
                ImGui::SetNextItemWidth(inputWidth - buttonWidth);
                if (ImGui::BeginCombo("##output_type", outputTypes[currentType])) {
                    for (int i = 0; i < IM_ARRAYSIZE(outputTypes); i++) {
                        const bool isSelected = (currentType == i);
                        if (ImGui::Selectable(outputTypes[i], isSelected)) {
                            currentType = i;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                // �ܹ�ѡ�񲿷�
                ImGui::AlignTextToFramePadding();
                ImGui::Text("x64:");
                ImGui::SameLine(labelWidth);

                static bool useX64 = true;
                ImGui::Checkbox("##use_x64", &useX64);

                // ���ɰ�ť����
                ImGui::Spacing();
                ImGui::Spacing();
                const float centerX = (ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f;
                ImGui::SetCursorPosX(centerX);
                if (ImGui::Button("Generate", ImVec2(buttonWidth, 0))) {
                    // ���ɲ����߼�
                    if (!selectListenerItem.empty()) {
                        // ��Ч�����ɲ���
                        FileSave::payloadSave(
                            selectListenerItem.back().name,
                            selectListenerItem.back().payload,
                            selectListenerItem.back().ip,
                            selectListenerItem.back().port);
                    }

                    window_states["windows"] = false;
                }
            }

            ImGui::End();
        }

        //����listenerWindow
        if (window_states["listenerWindow"]) {
            ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver); // ����Ĭ�ϴ��ڴ�С
            if (ImGui::Begin("listenerWindow", &window_states["listenerWindow"])) {
                // ��������
                ImGui::Text("Listener Configuration:");

                // ���Ƽ������Ķ�̬�б�
                static int listenerSelectedRow = -1; // ����ƴд����

                // �Ӵ��ڳߴ������Ԥ���ռ����ť
                ImGui::BeginChild("##listenerScrollingRegion", ImVec2(0, ImGui::GetContentRegionAvail().y - 30), true,
                    ImGuiWindowFlags_HorizontalScrollbar);

                ImGui::Columns(4, "##listenerViewColumns", true); // ����ƴд����

                // ������
                ImGui::Text("Name");      ImGui::NextColumn();
                ImGui::Text("Payload");   ImGui::NextColumn();
                ImGui::Text("IP");        ImGui::NextColumn();
                ImGui::Text("Port");      ImGui::NextColumn();
                ImGui::Separator();

                for (int idx = 0; idx < listenerItems.size(); ++idx) { // ����ƴд����
                    auto& listenerItem = listenerItems[idx]; // ����ƴд����
                    ImGui::PushID(listenerItem.name.c_str());

                    // ========== ��һ���������ɵ������ ==========
                    ImGui::Selectable("##listenerRow", listenerSelectedRow == idx,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);

                    // �����е���¼�
                    if (ImGui::IsItemHovered()) {
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            listenerSelectedRow = idx;
                        }
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            listenerSelectedRow = idx;
                            ImGui::OpenPopup("RowContextMenu");
                        }
                    }

                    // ========== �ڶ��������������� ==========
                    ImGui::SameLine(0, 0); // �� Selectable ����
                    ImGui::Text("%s", listenerItem.name.c_str());    ImGui::NextColumn();
                    ImGui::Text("%s", listenerItem.payload.c_str()); ImGui::NextColumn();
                    ImGui::Text("%s", listenerItem.ip.c_str());      ImGui::NextColumn();
                    ImGui::Text("%s", listenerItem.port.c_str());    ImGui::NextColumn();

                    // ========== �����������Ƹ���Ч�� ==========
                    if (listenerSelectedRow == idx) {
                        ImDrawList* draw_list = ImGui::GetWindowDrawList();
                        ImVec2 min = ImGui::GetItemRectMin();
                        ImVec2 max = ImGui::GetItemRectMax();
                        max.x = min.x + ImGui::GetContentRegionAvail().x; // ����������
                        draw_list->AddRectFilled(min, max,
                            ImGui::GetColorU32(ImGuiCol_Header), 0.0f);
                    }

                    // ========== ���Ĳ��������Ҽ��˵� ==========
                    //�رշ���������������δʵ��
                    if (ImGui::BeginPopup("RowContextMenu")) {
                        if (ImGui::MenuItem("Delete")) {
                            // ɾ��ѡ����
                            listenerItems.erase(listenerItems.begin() + idx);
                            listenerSelectedRow = -1;
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }

                ImGui::Columns(1);
                ImGui::EndChild(); // �����Ӵ���

                // �������հ�����ȡ��ѡ��
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                    ImGui::IsWindowHovered() &&
                    !ImGui::IsAnyItemHovered()) {
                    listenerSelectedRow = -1;
                }

                // ��ť����
                ImGui::Spacing();
                if (ImGui::Button("Choose", ImVec2(80, 0))) {
                    // ����ѡ���߼�
                    if (listenerSelectedRow >= 0) {
                        // TODO: ʵ��ѡ���߼�
                        selectListenerItem.push_back(listenerItems[listenerSelectedRow]);
                    }

                    window_states["listenerWindow"] = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Add", ImVec2(80, 0))) {
                    // ����¼�����
                    window_states["listenerAdd"] = true;
                }
            }
            ImGui::End();
        }

        if (window_states["listenerAdd"])
        {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("listenerAdd", &window_states["listenerAdd"])) {
                // ��������
                float labelWidth = 50.0f;  // ͳһ��ǩ��ȣ������λ
                float inputWidth = ImGui::GetContentRegionAvail().x - labelWidth;

                ImGui::Text("Listener Add Configuration:");
                const int listenerBufSize = 64;
                static char name[listenerBufSize] = "";
                static char payload[listenerBufSize] = "";
                static char ip[listenerBufSize] = "";
                static char port[listenerBufSize] = "";

                ImGui::Text("name:"); ImGui::SameLine(labelWidth);
                ImGui::SetNextItemWidth(inputWidth);
                ImGui::InputText("##listenerName", name, IM_ARRAYSIZE(name));


                static const char* payloadItems[] = { "tcp","http","dns" };
                static int currentPayloadItem = 0;
                strncpy_s(payload, payloadItems[currentPayloadItem], sizeof(payloadItems[currentPayloadItem]) - 1);
                payload[sizeof(payloadItems[currentPayloadItem])] = '\0';
                ImGui::Text("pyload:"); ImGui::SameLine(labelWidth);
                ImGui::SetNextItemWidth(inputWidth);
                if (ImGui::Combo("##payloadItems", &currentPayloadItem, payloadItems, IM_ARRAYSIZE(payloadItems))) {
                    int size = sizeof(payloadItems[currentPayloadItem]);
                    strncpy_s(payload, payloadItems[currentPayloadItem], size);
                    payload[size] = '\0';
                }
                ImGui::Text("ip:"); ImGui::SameLine(labelWidth);
                ImGui::SetNextItemWidth(inputWidth);
                ImGui::InputText("##listenerIp", ip, IM_ARRAYSIZE(ip));

                ImGui::Text("port:"); ImGui::SameLine(labelWidth);
                ImGui::SetNextItemWidth(inputWidth);
                ImGui::InputText("##listenerPort", port, IM_ARRAYSIZE(port));

                ImGui::Spacing();
                ImGui::Spacing();

                if (ImGui::Button("Save", ImVec2(50.0f, 0.0f))) {
                    // �������߼�
                    listenerItems.push_back(ListenerItem{ name, payload, ip, port });
                    window_states["listenerAdd"] = false;

                    std::string listenerAddData = stringCombination(
                        "listenerAdd:",
                        listenerItems.back().name,
                        ":",
                        listenerItems.back().payload,
                        ":",
                        listenerItems.back().ip,
                        ":",
                        listenerItems.back().port);

                    ProcessCommand((char*)listenerAddData.data(), authenticationManager);

                }
            }
            ImGui::End();
        }

        if (window_states["linux"]) {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Listener Setting", &window_states["linux"])) {
                // ��������
                ImGui::Text("HTTP Listener Configuration:");
                static char ip[16] = "0.0.0.0";
                static int port = 8081;
                ImGui::InputText("IP", ip, IM_ARRAYSIZE(ip));
                ImGui::InputInt("Port", &port);

                if (ImGui::Button("Generate")) {
                    // �������߼�

                }
            }
            ImGui::End();
        }

        if (window_states["android"]) {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Listener Setting", &window_states["android"])) {
                // ��������
                ImGui::Text("DNS Listener Configuration:");
                static char ip[16] = "0.0.0.0";
                static int port = 8081;
                ImGui::InputText("IP", ip, IM_ARRAYSIZE(ip));
                ImGui::InputInt("Port", &port);

                if (ImGui::Button("Generate")) {
                    // �������߼�

                }
            }
            ImGui::End();
        }

    }
}