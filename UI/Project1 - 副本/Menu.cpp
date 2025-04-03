#include <iostream>
#include <sstream>
#include <vector>
#include <Shlobj.h>

#include "Menu.h"
#include "PayloadGeneration.h"
#include "commandMenu.h"
#include "windowsDraw.h"
#include "ViewConnection.h"

void hello() {
    MessageBoxA(NULL, "To be developed...", "", 0);
}

namespace App {

    std::string GbkToUtf8(std::string& gbkStr) {
        // Step 1: GBK → WideChar (显式指定长度)
        int gbkLen = static_cast<int>(gbkStr.size());
        int wideLen = MultiByteToWideChar(936, 0, gbkStr.c_str(), gbkLen, nullptr, 0);
        if (wideLen <= 0) return "";

        std::wstring wideStr(wideLen, 0);
        if (MultiByteToWideChar(936, 0, gbkStr.c_str(), gbkLen, &wideStr[0], wideLen) == 0) {
            return "";
        }

        // Step 2: WideChar → UTF-8 (显式指定长度)
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), wideLen, nullptr, 0, nullptr, nullptr);
        if (utf8Len <= 0) return "";

        std::string utf8Str(utf8Len, 0);
        if (WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), wideLen, &utf8Str[0], utf8Len, nullptr, nullptr) == 0) {
            return "";
        }

        return utf8Str;
    }

    //static bool show_tcp_window = false;
    static std::unordered_map<std::string, bool> window_states = {
        {"windows", false},
        {"linux", false},
        {"android", false},
        {"listenerWindow",false},
        {"listenerAdd",false}
    };
    
    //通过接口提供外部cpp文件访问，便于封装
    std::unordered_map<std::string, bool>& getWindowsStates() {
        return window_states;
    }

    static float splitter_ratio = 0.5f;       // 分割比例
    
    float& getSplitterRatio() {
        return splitter_ratio;
    }
    
    static bool dragging_splitter = false;     // 拖拽状态
    bool& getDraggingSplitter() {
        return dragging_splitter;
    }

    static std::vector<ContentItem> items;  // 存储解析结果
    static std::vector<ListenerItem> listenerItems;
    static std::vector<ListenerItem> selectListenerItem ;

    std::vector<ContentItem>& getContentItems() {
        return items;
    }

    std::vector<ListenerItem>& getListenerItems() {
        return listenerItems;
    }
    
    std::vector<ListenerItem>& getSelectListenerItem() {
        return selectListenerItem;
    }

    void parseString(const std::string& input) {
        
        items.clear();

        std::stringstream ss(input);
        std::string token;
        int index = 0;
        std::string id = "";
        std::string ip = "";

        // 按 `:` 拆分字符串
        while (std::getline(ss, token, ':')) {
            if (token.empty() || token == "puppet") {
                continue;
            }
            if (0 == index) {
                id = token;
                index = 1;
            }
            else
            {
                ip = token;
                index = 0;

                ContentItem contentItem;
                contentItem.id = id;
                contentItem.ip = ip;
                contentItem.note = "DefaultNote";
                contentItem.user = "DefaultUser";
                contentItem.computer = "DefaultPC";
                contentItem.selected = false;

                items.push_back(contentItem);
            }
        }
    }

    void RenderUI(AuthenticationManager* authenticationManager) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 viewport_pos = viewport->Pos;
        ImVec2 viewport_size = viewport->Size;

        // 计算各区域高度
        float nav_height = ImGui::GetFrameHeightWithSpacing() * 1.5f; // 导航栏高度
        float cmd_height = ImGui::GetFrameHeightWithSpacing() * 1.5f; // 命令栏高度
        float total_view_height = viewport_size.y - nav_height - cmd_height;

        // 导航栏窗口 ------------------------------------------------------
        ImGui::SetNextWindowPos(viewport_pos);
        ImGui::SetNextWindowSize(ImVec2(viewport_size.x, nav_height));
        ImGuiWindowFlags nav_flags =
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_MenuBar;
        // 设置导航栏窗口的背景颜色和菜单栏颜色
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.2f, 0.5f, 0.8f, 0.8f));
        ImGui::Begin("Navigation Bar:", nullptr, nav_flags);
        if (ImGui::BeginMenuBar()) {
            // 菜单项代码保持不变...
            if (ImGui::BeginMenu("Visualization")) {

                if(ImGui::MenuItem("connectionManager")) {
                    hello();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Payload")) {

                if (ImGui::MenuItem("windows")) {
                    
                    window_states["windows"] = true;
                }
                
                if (ImGui::MenuItem("linux")) {

                    window_states["linux"] = true;
                }
                
                if (ImGui::MenuItem("android")) {

                    window_states["android"] = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Evasion")) {

                if (ImGui::MenuItem("create")) {
                    hello();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window")) {

                if (ImGui::MenuItem("Restore the default settings")) {
                    hello();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools")) {

                if (ImGui::MenuItem("fish")) {
                    hello();
                }

                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("NytrixAI")) {

                if (ImGui::MenuItem("ai")) {
                    hello();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Preferences")) {

                if (ImGui::MenuItem("setting")) {
                    hello();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {

                if (ImGui::MenuItem("Document")) {

                    hello();
                }

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
        ImGui::PopStyleColor();

        //绘制各种菜单窗体
        windowsDrawing(authenticationManager);

        // 命令栏窗口 ------------------------------------------------------
        commandWindowDrawing(viewport_pos.x, viewport_pos.y, viewport_size.x, viewport_size.y,authenticationManager);

        // 视图窗口系统 --------------------------------------------------
        // 计算视图区域布局
        viewConnection(viewport_pos.x, viewport_pos.y, viewport_size.x, total_view_height,authenticationManager);

    }
}