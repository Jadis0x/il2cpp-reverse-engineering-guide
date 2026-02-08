#include "pch-il2cpp.h"
#include "gui/GUITheme.h"
#include <imgui/imgui.h>

void ApplyTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    const ImVec4 BgColor = ImVec4(0.05f, 0.05f, 0.05f, 0.50f);
    const ImVec4 ChildBg = ImVec4(0.08f, 0.08f, 0.08f, 0.50f);
    const ImVec4 Border = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); 
    const ImVec4 Text = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); 
    const ImVec4 TextDis = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);

    const ImVec4 Accent = ImVec4(0.18f, 0.30f, 0.50f, 1.00f);
    const ImVec4 AccentHover = ImVec4(0.22f, 0.38f, 0.62f, 1.00f);
    const ImVec4 AccentActive = ImVec4(0.26f, 0.46f, 0.75f, 1.00f);

    colors[ImGuiCol_Text] = Text;
    colors[ImGuiCol_TextDisabled] = TextDis;
    colors[ImGuiCol_WindowBg] = BgColor;
    colors[ImGuiCol_ChildBg] = ChildBg;
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);

    colors[ImGuiCol_Border] = Border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.80f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = Accent;

    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f); 
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.80f);

    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.50f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = AccentHover;
    colors[ImGuiCol_ScrollbarGrabActive] = AccentActive;

    colors[ImGuiCol_CheckMark] = AccentActive;
    colors[ImGuiCol_SliderGrab] = Accent;
    colors[ImGuiCol_SliderGrabActive] = AccentActive;

    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.12f, 0.60f); 
    colors[ImGuiCol_ButtonHovered] = AccentHover;
    colors[ImGuiCol_ButtonActive] = AccentActive;

    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.15f, 0.80f);
    colors[ImGuiCol_HeaderHovered] = AccentHover;
    colors[ImGuiCol_HeaderActive] = Accent;

    colors[ImGuiCol_Separator] = Border;
    colors[ImGuiCol_SeparatorHovered] = Accent;
    colors[ImGuiCol_SeparatorActive] = AccentActive;

    colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.20f, 0.20f, 0.40f);
    colors[ImGuiCol_ResizeGripHovered] = Accent;
    colors[ImGuiCol_ResizeGripActive] = AccentActive;

    colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.08f, 0.00f);
    colors[ImGuiCol_TabHovered] = AccentHover;
    colors[ImGuiCol_TabActive] = Accent;
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.08f, 0.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 5);
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 0.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    style.TouchExtraPadding = ImVec2(2.0f, 2.0f);
}