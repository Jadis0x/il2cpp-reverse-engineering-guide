#include "pch-il2cpp.h"

#define IMGUI_DEFINE_MATH_OPERATORS 

#include "menu.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "settings.h"
#include "gui/tabs/SettingsTAB.h"
#include "gui/tabs/UnityExplorerTAB.h"

static bool SidebarButton(const char* label, bool active, const ImVec2& size_arg = ImVec2(0, 0))
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImU32 bg_col = ImGui::GetColorU32(active ? ImGuiCol_HeaderActive : (hovered ? ImGuiCol_HeaderHovered : ImGuiCol_WindowBg));

    if (active || hovered)
        window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col, 0.0f);

    if (active) {
        window->DrawList->AddRectFilled(
            ImVec2(bb.Min.x, bb.Min.y),
            ImVec2(bb.Min.x + 3, bb.Max.y),
            ImGui::GetColorU32(ImVec4(0.4f, 0.7f, 1.0f, 1.0f))
        );
    }

    ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

    return pressed;
}

namespace Menu {
    bool init = false;
    int currentTab = 0; // 0: Explorer, 1: Settings, 2: Inspector

    void Init() {
        ImGui::SetNextWindowSize(ImVec2(1000, 650), ImGuiCond_FirstUseEver);
        init = true;
    }

    void Render() {
        if (!init) Menu::Init();

        static int lastTab = currentTab;
        if (currentTab != lastTab) {
            if (currentTab == 0) UnityExplorerTAB::Reset();
            lastTab = currentTab;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("Il2CppInspectorPro by Jadis0x", &settings.bShowMenu, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::PopStyleVar();

            ImGui::Columns(2, "MainLayout", false);
            static bool firstRun = true;
            if (firstRun) {
                ImGui::SetColumnWidth(0, 150.0f);
                firstRun = false;
            }

            ImGui::BeginChild("Sidebar", ImVec2(0, 0), false);
            {
                ImGui::Spacing();
                ImGui::Indent(10.0f); ImGui::TextDisabled("GENERAL"); ImGui::Unindent(10.0f);

                if (SidebarButton("Unity Explorer", currentTab == 0, ImVec2(-1, 35))) currentTab = 0;
                if (SidebarButton("Settings", currentTab == 1, ImVec2(-1, 35))) currentTab = 1;
            }
            ImGui::EndChild();
            ImGui::NextColumn();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
            ImGui::BeginChild("Content", ImVec2(0, 0), true);
            {
                switch (currentTab) {
                case 0: UnityExplorerTAB::Render(); break;
                case 1: SettingsTAB::Render(); break;
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }
        ImGui::End();
    }
} 