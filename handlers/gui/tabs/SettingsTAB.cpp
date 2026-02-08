#include "pch-il2cpp.h"

#include <iostream>
#include "gui/tabs/SettingsTAB.h"
#include "gui/GUITheme.h" 
#include "settings.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "main.h"

#include "Il2CppResolver.h"

static void TextCentered(std::string text) {
	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = ImGui::CalcTextSize(text.c_str()).x;
	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	ImGui::Text("%s", text.c_str());
}

static void DrawRainbowTitle(const char* text) {
	static float t = 0.0f;
	t += ImGui::GetIO().DeltaTime * 0.5f; 

	ImVec4 color;
	color.x = sin(t) * 0.5f + 0.5f;
	color.y = sin(t + 2.0f) * 0.5f + 0.5f;
	color.z = sin(t + 4.0f) * 0.5f + 0.5f;
	color.w = 1.0f;

	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = ImGui::CalcTextSize(text).x;

	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	ImGui::TextColored(color, "%s", text);
	ImGui::Separator();
}

void SettingsTAB::Render()
{
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::Spacing();
    ImGui::SetWindowFontScale(1.2f); 
    DrawRainbowTitle("Il2CppInspectorPro");
    ImGui::SetWindowFontScale(1.0f); 
    ImGui::Spacing();

    ImGui::Columns(2, "DashCols", false); 

    ImGui::BeginChild("InfoCard", ImVec2(0, 100), true);
    {
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "BUILD INFO");
        ImGui::Separator();
        ImGui::Text("Version: 1.0.0 (Beta)");
        ImGui::TextDisabled("Build Date: Jan 2026");
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    // FPS GRAPH
    ImGui::BeginChild("PerfCard", ImVec2(0, 100), true);
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "PERFORMANCE");
        static float values[90] = {};
        static int values_offset = 0;
        static double refresh_time = 0.0f;

        if (refresh_time == 0.0f) refresh_time = ImGui::GetTime();
        while (refresh_time < ImGui::GetTime()) {
            values[values_offset] = ImGui::GetIO().Framerate;
            values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
            refresh_time += 1.0f / 60.0f;
        }

        char overlay[32];
        sprintf_s(overlay, "FPS: %.0f", ImGui::GetIO().Framerate);

        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
        ImGui::PlotLines("##fps", values, IM_ARRAYSIZE(values), values_offset, overlay, 0.0f, 240.0f, ImVec2(ImGui::GetContentRegionAvail().x, 60));
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::Columns(1);
    ImGui::Spacing();


    ImGui::BeginChild("Socials", ImVec2(0, 40), false);
    {
        float width = ImGui::GetWindowWidth();
        float buttonWidth = 100;
        ImGui::SetCursorPosX((width - (buttonWidth * 3 + 20)) * 0.5f);

        if (ImGui::Button("Discord", ImVec2(buttonWidth, 0))) {
            // your discord link..
        }
        ImGui::SameLine();
        if (ImGui::Button("GitHub", ImVec2(buttonWidth, 0))) {
            Resolver::Helpers::OpenURL("https://www.github.com/jadis0x");
        }
        ImGui::SameLine();
        if (ImGui::Button("Donate", ImVec2(buttonWidth, 0))) {
            Resolver::Helpers::OpenURL("https://www.buymeacoffee.com/jadis0x");
        }
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("CONFIGURATION");

    if (ImGui::BeginTable("SettingsTable", 2)) {
        ImGui::TableNextColumn();
        ImGui::Checkbox("Show Unity Logs", &settings.bEnableUnityLogs);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enables the internal console logger.");

        ImGui::TableNextColumn();
       
        ImGui::TableNextColumn();
        static ImVec4 color = style.Colors[ImGuiCol_Border];
        if (ImGui::ColorEdit3("Accent Color", (float*)&color, ImGuiColorEditFlags_NoInputs)) {
            // THIS IS TEST. YOU CAN REMOVE IT
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("Unload DLL", ImVec2(-1, 30)))
    {
        SetEvent(hUnloadEvent);
    }

    ImGui::PopStyleColor(3);

    ImGui::Spacing();
    TextCentered("Made with <3 by Jadis0x");
}
