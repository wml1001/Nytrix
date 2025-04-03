//ViewConnection.cpp
#include "ViewConnection.h"
#include "Menu.h"
//#include "imGUI/imgui_internal.h"

namespace App {

	void viewConnection(int x,int y,int width,int total_view_height, AuthenticationManager* authenticationManager) {
        // 视图窗口系统 --------------------------------------------------
        // 计算视图区域布局
        std::vector<ContentItem>& items = getContentItems();
        float& splitter_ratio = getSplitterRatio();
        bool& dragging_splitter = getDraggingSplitter();
        float nav_height = ImGui::GetFrameHeightWithSpacing() * 1.5f; // 导航栏高度

        ImVec2 view1_pos(x, y + nav_height);
        float view1_height = total_view_height * splitter_ratio;
        view1_height = ImClamp(view1_height, MIN_VIEW_HEIGHT, total_view_height - MIN_VIEW_HEIGHT - SPLITTER_HEIGHT);

        // View1 窗口
        ImGui::SetNextWindowPos(view1_pos);
        ImGui::SetNextWindowSize(ImVec2(width, view1_height));
        ImGui::Begin("View1", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

        parseString(authenticationManager->getpuppetInfo());

        static int selected_row = -1;  // 当前选中的行索引
        static int context_row = -1;   // 右键点击的行索引

        ImGui::BeginChild("##scrolling_region", ImVec2(0, 0), false,
            ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::Columns(5, "##view1_columns", false);

        // 标题行
        ImGui::Text("ID");      ImGui::NextColumn();
        ImGui::Text("IP");      ImGui::NextColumn();
        ImGui::Text("Note");    ImGui::NextColumn();
        ImGui::Text("User");    ImGui::NextColumn();
        ImGui::Text("Computer"); ImGui::NextColumn();
        ImGui::Separator();

        for (int idx = 0; idx < items.size(); ++idx) {
            auto& item = items[idx];
            ImGui::PushID(item.id.c_str());

            // ========== 第一步：创建可点击区域（作为底层） ==========
            ImGui::Selectable("##row", selected_row == idx,
                ImGuiSelectableFlags_SpanAllColumns |
                ImGuiSelectableFlags_AllowItemOverlap);

            // 处理行点击事件
            if (ImGui::IsItemHovered()) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    selected_row = idx;
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    context_row = idx;
                    ImGui::OpenPopup("RowContextMenu");
                }
            }

            // ========== 第二步：绘制内容（叠加在点击区域之上） ==========
            ImGui::SameLine(0, 0); // 与Selectable对齐
            ImGui::Text("%s", item.id.c_str()); ImGui::NextColumn();
            ImGui::Text("%s", item.ip.c_str()); ImGui::NextColumn();

            // 可编辑的Note列（最小化视觉表现）
            char note_buffer[256] = { 0 };
            item.note.copy(note_buffer, sizeof(note_buffer) - 1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);       // 透明背景
            ImGui::PushStyleColor(ImGuiCol_Border, 0);        // 无边框
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); // 紧凑布局
            if (ImGui::InputText("##note_edit", note_buffer, sizeof(note_buffer),
                ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll))
            {
                item.note = note_buffer;
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
            ImGui::NextColumn();

            ImGui::Text("%s", item.user.c_str()); ImGui::NextColumn();
            ImGui::Text("%s", item.computer.c_str()); ImGui::NextColumn();

            // ========== 第三步：绘制高亮效果（使用DrawList覆盖） ==========
            if (selected_row == idx) {
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                max.x = min.x + ImGui::GetContentRegionAvail().x; // 跨所有列
                draw_list->AddRectFilled(min, max,
                    ImGui::GetColorU32(ImGuiCol_Header), 0.0f);
            }

            // ========== 第四步：右键菜单 ==========
            if (context_row == idx && ImGui::BeginPopup("RowContextMenu")) {
                std::string setBuf = "";

                if (ImGui::MenuItem("Interact")) {

                    setBuf = "set ";
                    setBuf.append(item.id);
                    ProcessCommand((char*)setBuf.c_str(), authenticationManager);
                }

                if (ImGui::MenuItem("Delete")) {
                    setBuf = "set ";
                    setBuf.append(item.id);
                    ProcessCommand((char*)setBuf.c_str(), authenticationManager);

                    Sleep(100);
                    setBuf = "exit";
                    ProcessCommand((char*)setBuf.c_str(), authenticationManager);
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::EndChild();
        ImGui::End();

        // 分隔条交互区域
        ImVec2 splitter_pos(view1_pos.x, view1_pos.y + view1_height);
        ImVec2 splitter_size(width, SPLITTER_HEIGHT);

        ImGui::SetNextWindowPos(splitter_pos);
        ImGui::SetNextWindowSize(splitter_size);
        ImGui::Begin("##Splitter", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoSavedSettings);

        // 处理分隔条拖拽
        ImVec2 rect_min = splitter_pos;
        ImVec2 rect_max = ImVec2(splitter_pos.x + splitter_size.x,
            splitter_pos.y + splitter_size.y);
        bool hovered = ImGui::IsMouseHoveringRect(rect_min, rect_max);

        if (hovered || dragging_splitter)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

        if (!dragging_splitter && hovered && ImGui::IsMouseClicked(0))
            dragging_splitter = true;

        if (dragging_splitter) {
            if (ImGui::IsMouseDragging(0)) {
                float delta = ImGui::GetMouseDragDelta(0).y;
                splitter_ratio += delta / total_view_height;
                splitter_ratio = ImClamp(splitter_ratio,
                    MIN_VIEW_HEIGHT / total_view_height,
                    1.0f - (MIN_VIEW_HEIGHT + SPLITTER_HEIGHT) / total_view_height);
                ImGui::ResetMouseDragDelta(0);
            }

            if (!ImGui::IsMouseDown(0))
                dragging_splitter = false;
        }
        ImGui::End();

        // View2 窗口
        ImVec2 view2_pos(splitter_pos.x, splitter_pos.y + SPLITTER_HEIGHT);
        float view2_height = total_view_height - view1_height - SPLITTER_HEIGHT;

        ImGui::SetNextWindowPos(view2_pos);
        ImGui::SetNextWindowSize(ImVec2(width, view2_height));
        ImGui::Begin("View2", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        // 添加view2内容...
        std::string viewContent = authenticationManager->getReceivedData();

        std::string viewContentByGBK = GbkToUtf8(viewContent);

        ImGui::TextUnformatted(viewContentByGBK.c_str(),viewContentByGBK.c_str()+viewContentByGBK.size());
        ImGui::End();

	}
}