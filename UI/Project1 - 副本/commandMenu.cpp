#include "commandMenu.h"

void commandWindowDrawing(int x, int y, int width, int height, AuthenticationManager* authenticationManager) {
    // 命令栏窗口 ------------------------------------------------------
    const int CMD_SIZE = 4096;
    float cmd_height = ImGui::GetFrameHeightWithSpacing() * 1.5f; // 命令栏高度

    ImVec2 cmd_pos(x, y + height - cmd_height);
    //ImVec2 cmd_pos(viewport_pos.x, viewport_pos.y + viewport_size.y - cmd_height);
    ImGui::SetNextWindowPos(cmd_pos);
    ImGui::SetNextWindowSize(ImVec2(width, cmd_height));
    ImGui::Begin("Command:", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    char buf[CMD_SIZE] = { 0 };
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    // 添加一个静态变量跟踪是否需要设置焦点
    static bool focus_input = false;

    // 使用 InputTextWithHint 的唯一标识符
    ImGui::PushID("##cmd");
    if (ImGui::InputTextWithHint("##cmd", "Enter command...", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (strlen(buf) > 0)
        {
            ProcessCommand(buf, authenticationManager);
            buf[0] = '\0'; // 清空输入缓冲
            focus_input = true; // 标记需要设置焦点
        }
    }

    // 在输入框被提交后，立即请求焦点
    if (focus_input)
    {
        ImGui::SetKeyboardFocusHere(-1); // -1 表示前一个控件
        focus_input = false;
    }
    ImGui::PopID();

    ImGui::End();
}