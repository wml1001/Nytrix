//ViewConnection.cpp
#include "ViewConnection.h"
#include "Menu.h"
//#include "imGUI/imgui_internal.h"

namespace App {

	void viewConnection(int x,int y,int width,int total_view_height, AuthenticationManager* authenticationManager) {
        // ��ͼ����ϵͳ --------------------------------------------------
        // ������ͼ���򲼾�
        std::vector<ContentItem>& items = getContentItems();
        float& splitter_ratio = getSplitterRatio();
        bool& dragging_splitter = getDraggingSplitter();
        float nav_height = ImGui::GetFrameHeightWithSpacing() * 1.5f; // �������߶�

        ImVec2 view1_pos(x, y + nav_height);
        float view1_height = total_view_height * splitter_ratio;
        view1_height = ImClamp(view1_height, MIN_VIEW_HEIGHT, total_view_height - MIN_VIEW_HEIGHT - SPLITTER_HEIGHT);

        // View1 ����
        ImGui::SetNextWindowPos(view1_pos);
        ImGui::SetNextWindowSize(ImVec2(width, view1_height));
        ImGui::Begin("View1", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

        parseString(authenticationManager->getpuppetInfo());

        static int selected_row = -1;  // ��ǰѡ�е�������
        static int context_row = -1;   // �Ҽ������������

        ImGui::BeginChild("##scrolling_region", ImVec2(0, 0), false,
            ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::Columns(5, "##view1_columns", false);

        // ������
        ImGui::Text("ID");      ImGui::NextColumn();
        ImGui::Text("IP");      ImGui::NextColumn();
        ImGui::Text("Note");    ImGui::NextColumn();
        ImGui::Text("User");    ImGui::NextColumn();
        ImGui::Text("Computer"); ImGui::NextColumn();
        ImGui::Separator();

        for (int idx = 0; idx < items.size(); ++idx) {
            auto& item = items[idx];
            ImGui::PushID(item.id.c_str());

            // ========== ��һ���������ɵ��������Ϊ�ײ㣩 ==========
            ImGui::Selectable("##row", selected_row == idx,
                ImGuiSelectableFlags_SpanAllColumns |
                ImGuiSelectableFlags_AllowItemOverlap);

            // �����е���¼�
            if (ImGui::IsItemHovered()) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    selected_row = idx;
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    context_row = idx;
                    ImGui::OpenPopup("RowContextMenu");
                }
            }

            // ========== �ڶ������������ݣ������ڵ������֮�ϣ� ==========
            ImGui::SameLine(0, 0); // ��Selectable����
            ImGui::Text("%s", item.id.c_str()); ImGui::NextColumn();
            ImGui::Text("%s", item.ip.c_str()); ImGui::NextColumn();

            // �ɱ༭��Note�У���С���Ӿ����֣�
            char note_buffer[256] = { 0 };
            item.note.copy(note_buffer, sizeof(note_buffer) - 1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);       // ͸������
            ImGui::PushStyleColor(ImGuiCol_Border, 0);        // �ޱ߿�
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); // ���ղ���
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

            // ========== �����������Ƹ���Ч����ʹ��DrawList���ǣ� ==========
            if (selected_row == idx) {
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                max.x = min.x + ImGui::GetContentRegionAvail().x; // ��������
                draw_list->AddRectFilled(min, max,
                    ImGui::GetColorU32(ImGuiCol_Header), 0.0f);
            }

            // ========== ���Ĳ����Ҽ��˵� ==========
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

        // �ָ�����������
        ImVec2 splitter_pos(view1_pos.x, view1_pos.y + view1_height);
        ImVec2 splitter_size(width, SPLITTER_HEIGHT);

        ImGui::SetNextWindowPos(splitter_pos);
        ImGui::SetNextWindowSize(splitter_size);
        ImGui::Begin("##Splitter", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoSavedSettings);

        // ����ָ�����ק
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

        // View2 ����
        ImVec2 view2_pos(splitter_pos.x, splitter_pos.y + SPLITTER_HEIGHT);
        float view2_height = total_view_height - view1_height - SPLITTER_HEIGHT;

        ImGui::SetNextWindowPos(view2_pos);
        ImGui::SetNextWindowSize(ImVec2(width, view2_height));
        ImGui::Begin("View2", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        // ���view2����...
        std::string viewContent = authenticationManager->getReceivedData();

        std::string viewContentByGBK = GbkToUtf8(viewContent);

        ImGui::TextUnformatted(viewContentByGBK.c_str(),viewContentByGBK.c_str()+viewContentByGBK.size());
        ImGui::End();

	}
}