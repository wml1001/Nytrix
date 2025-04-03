#include "commandMenu.h"

void commandWindowDrawing(int x, int y, int width, int height, AuthenticationManager* authenticationManager) {
    // ���������� ------------------------------------------------------
    const int CMD_SIZE = 4096;
    float cmd_height = ImGui::GetFrameHeightWithSpacing() * 1.5f; // �������߶�

    ImVec2 cmd_pos(x, y + height - cmd_height);
    //ImVec2 cmd_pos(viewport_pos.x, viewport_pos.y + viewport_size.y - cmd_height);
    ImGui::SetNextWindowPos(cmd_pos);
    ImGui::SetNextWindowSize(ImVec2(width, cmd_height));
    ImGui::Begin("Command:", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    char buf[CMD_SIZE] = { 0 };
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    // ���һ����̬���������Ƿ���Ҫ���ý���
    static bool focus_input = false;

    // ʹ�� InputTextWithHint ��Ψһ��ʶ��
    ImGui::PushID("##cmd");
    if (ImGui::InputTextWithHint("##cmd", "Enter command...", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (strlen(buf) > 0)
        {
            ProcessCommand(buf, authenticationManager);
            buf[0] = '\0'; // ������뻺��
            focus_input = true; // �����Ҫ���ý���
        }
    }

    // ��������ύ���������󽹵�
    if (focus_input)
    {
        ImGui::SetKeyboardFocusHere(-1); // -1 ��ʾǰһ���ؼ�
        focus_input = false;
    }
    ImGui::PopID();

    ImGui::End();
}