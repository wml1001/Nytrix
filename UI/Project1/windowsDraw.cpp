#include "windowsDraw.h"
#include "Menu.h"
#include "utils.h"
#include "PayloadGeneration.h"

namespace App {

    void windowsDrawing(AuthenticationManager* authenticationManager){

        std::unordered_map<std::string, bool>&  window_states = getWindowsStates();
        std::vector<ListenerItem>& listenerItems = getListenerItems();
        std::vector<ListenerItem>& selectListenerItem = getSelectListenerItem();
        //绘制payload窗
        if (window_states["windows"]) {

            //绘制payload选择窗口
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

                // 将 generateListener 中最后一个元素的 name 属性赋值给 listenerBuf
                char listenerBuf[BUFSIZ] = { 0 };
                if (!generateListener.empty()) {
                    strncpy_s(listenerBuf, generateListener.back().name.c_str(), BUFSIZ - 1);
                    listenerBuf[BUFSIZ - 1] = '\0'; // 确保字符串以空字符结尾
                    generateListener.pop_back();
                }

                // 统一控件样式参数
                static const float buttonWidth = 80.0f;

                // 监听器设置部分
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Listener:");
                ImGui::SameLine(labelWidth);

                // 使用输入框显示监听器信息（只读模式）
                ImGui::SetNextItemWidth(inputWidth - buttonWidth);
                ImGui::InputText("##listener", listenerBuf, IM_ARRAYSIZE(listenerBuf),
                    ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);

                // 选择按钮与输入框保持同一行
                ImGui::SameLine();
                if (ImGui::Button("Choose", ImVec2(buttonWidth, 0))) {
                    window_states["listenerWindow"] = true;
                }

                // 输出类型选择部分
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

                // 架构选择部分
                ImGui::AlignTextToFramePadding();
                ImGui::Text("x64:");
                ImGui::SameLine(labelWidth);

                static bool useX64 = true;
                ImGui::Checkbox("##use_x64", &useX64);

                // 生成按钮布局
                ImGui::Spacing();
                ImGui::Spacing();
                const float centerX = (ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f;
                ImGui::SetCursorPosX(centerX);
                if (ImGui::Button("Generate", ImVec2(buttonWidth, 0))) {
                    // 生成操作逻辑
                    if (!selectListenerItem.empty()) {
                        // 有效的生成操作
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

        //绘制listenerWindow
        if (window_states["listenerWindow"]) {
            ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver); // 调整默认窗口大小
            if (ImGui::Begin("listenerWindow", &window_states["listenerWindow"])) {
                // 窗口内容
                ImGui::Text("Listener Configuration:");

                // 绘制监听器的动态列表
                static int listenerSelectedRow = -1; // 修正拼写错误

                // 子窗口尺寸调整，预留空间给按钮
                ImGui::BeginChild("##listenerScrollingRegion", ImVec2(0, ImGui::GetContentRegionAvail().y - 30), true,
                    ImGuiWindowFlags_HorizontalScrollbar);

                ImGui::Columns(4, "##listenerViewColumns", true); // 修正拼写错误

                // 标题行
                ImGui::Text("Name");      ImGui::NextColumn();
                ImGui::Text("Payload");   ImGui::NextColumn();
                ImGui::Text("IP");        ImGui::NextColumn();
                ImGui::Text("Port");      ImGui::NextColumn();
                ImGui::Separator();

                for (int idx = 0; idx < listenerItems.size(); ++idx) { // 修正拼写错误
                    auto& listenerItem = listenerItems[idx]; // 修正拼写错误
                    ImGui::PushID(listenerItem.name.c_str());

                    // ========== 第一步：创建可点击区域 ==========
                    ImGui::Selectable("##listenerRow", listenerSelectedRow == idx,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);

                    // 处理行点击事件
                    if (ImGui::IsItemHovered()) {
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            listenerSelectedRow = idx;
                        }
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            listenerSelectedRow = idx;
                            ImGui::OpenPopup("RowContextMenu");
                        }
                    }

                    // ========== 第二步：绘制行内容 ==========
                    ImGui::SameLine(0, 0); // 与 Selectable 对齐
                    ImGui::Text("%s", listenerItem.name.c_str());    ImGui::NextColumn();
                    ImGui::Text("%s", listenerItem.payload.c_str()); ImGui::NextColumn();
                    ImGui::Text("%s", listenerItem.ip.c_str());      ImGui::NextColumn();
                    ImGui::Text("%s", listenerItem.port.c_str());    ImGui::NextColumn();

                    // ========== 第三步：绘制高亮效果 ==========
                    if (listenerSelectedRow == idx) {
                        ImDrawList* draw_list = ImGui::GetWindowDrawList();
                        ImVec2 min = ImGui::GetItemRectMin();
                        ImVec2 max = ImGui::GetItemRectMax();
                        max.x = min.x + ImGui::GetContentRegionAvail().x; // 覆盖所有列
                        draw_list->AddRectFilled(min, max,
                            ImGui::GetColorU32(ImGuiCol_Header), 0.0f);
                    }

                    // ========== 第四步：定义右键菜单 ==========
                    //关闭服务器监听功能暂未实现
                    if (ImGui::BeginPopup("RowContextMenu")) {
                        if (ImGui::MenuItem("Delete")) {
                            // 删除选中行
                            listenerItems.erase(listenerItems.begin() + idx);
                            listenerSelectedRow = -1;
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }

                ImGui::Columns(1);
                ImGui::EndChild(); // 结束子窗口

                // 处理点击空白区域取消选中
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                    ImGui::IsWindowHovered() &&
                    !ImGui::IsAnyItemHovered()) {
                    listenerSelectedRow = -1;
                }

                // 按钮区域
                ImGui::Spacing();
                if (ImGui::Button("Choose", ImVec2(80, 0))) {
                    // 处理选中逻辑
                    if (listenerSelectedRow >= 0) {
                        // TODO: 实现选中逻辑
                        selectListenerItem.push_back(listenerItems[listenerSelectedRow]);
                    }

                    window_states["listenerWindow"] = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Add", ImVec2(80, 0))) {
                    // 添加新监听器
                    window_states["listenerAdd"] = true;
                }
            }
            ImGui::End();
        }

        if (window_states["listenerAdd"])
        {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("listenerAdd", &window_states["listenerAdd"])) {
                // 窗口内容
                float labelWidth = 50.0f;  // 统一标签宽度，避免错位
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
                    // 监听器逻辑
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
                // 窗口内容
                ImGui::Text("HTTP Listener Configuration:");
                static char ip[16] = "0.0.0.0";
                static int port = 8081;
                ImGui::InputText("IP", ip, IM_ARRAYSIZE(ip));
                ImGui::InputInt("Port", &port);

                if (ImGui::Button("Generate")) {
                    // 监听器逻辑

                }
            }
            ImGui::End();
        }

        if (window_states["android"]) {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Listener Setting", &window_states["android"])) {
                // 窗口内容
                ImGui::Text("DNS Listener Configuration:");
                static char ip[16] = "0.0.0.0";
                static int port = 8081;
                ImGui::InputText("IP", ip, IM_ARRAYSIZE(ip));
                ImGui::InputInt("Port", &port);

                if (ImGui::Button("Generate")) {
                    // 监听器逻辑

                }
            }
            ImGui::End();
        }

    }
}