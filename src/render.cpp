#include "render.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "offsets.hpp"
#include "outline.h"
#include "triggerbot.h"
#include "ulttracker.h"
#include "features.hpp"
#include "config.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

static void Toggle(const char* label, bool* v)
{
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) return;
    ImGuiContext& g = *GImGui;
    ImGuiID id = w->GetID(label);
    ImVec2 pos = w->DC.CursorPos;
    float height = 18.f, width = 36.f;
    ImRect rect(pos, ImVec2(pos.x + width, pos.y + height));
    ImGui::ItemSize(rect);
    if (!ImGui::ItemAdd(rect, id)) return;
    float* anim = ImGui::GetStateStorage()->GetFloatRef(id, 0.f);
    float tgt = *v ? 1.f : 0.f;
    *anim += (tgt - *anim) * g.IO.DeltaTime * 14.f;
    if (fabsf(*anim - tgt) < 0.005f) *anim = tgt;
    auto* dl = ImGui::GetWindowDrawList();
    float r = height * 0.5f;
    float mix = *anim;
    ImU32 bgCol = ImGui::GetColorU32(ImVec4(0.08f + (0.f - 0.08f) * mix, 0.08f + (0.78f - 0.08f) * mix, 0.12f + (1.f - 0.12f) * mix, 0.9f));
    dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bgCol, r);
    if (*anim > 0.01f)
        dl->AddRectFilled(pos, ImVec2(pos.x + width * *anim, pos.y + height), ImGui::GetColorU32(ImVec4(0.f, 0.78f, 1.f, *anim * 0.25f)), r);
    float cx = pos.x + r + (width - r * 2) * *anim;
    dl->AddCircleFilled(ImVec2(cx, pos.y + r), r - 3.f, IM_COL32(255, 255, 255, 240));
    ImVec2 lp(pos.x + width + 9.f, pos.y + 1.f);
    dl->AddText(lp, ImGui::GetColorU32(ImVec4(0.86f, 0.86f, 0.9f, 1.f)), label);
    if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max) && ImGui::IsMouseClicked(0)) *v = !*v;
}

void RenderMenu() {
    static int tab = 0;
    static float tabAnim = 0.f;
    float tgt = (float)tab;
    tabAnim += (tgt - tabAnim) * ImGui::GetIO().DeltaTime * 10.f;
    if (fabsf(tabAnim - tgt) < 0.01f) tabAnim = tgt;

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.04f, 0.07f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.04f, 0.04f, 0.07f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.04f, 0.04f, 0.07f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.04f, 0.04f, 0.07f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.f, 0.78f, 1.f, 0.2f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));

    ImGui::Begin(S("RsExternal"), nullptr, ImGuiWindowFlags_NoCollapse);
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 wPos = ImGui::GetWindowPos(), wSize = ImGui::GetWindowSize();

    char entC[32]; sprintf(entC, S("Entities: %zu"), g_entities.size());
    ImVec2 entS = ImGui::CalcTextSize(entC);
    dl->AddText(ImVec2(wPos.x + wSize.x - 16 - entS.x, wPos.y + 8), IM_COL32(130, 130, 150, 200), entC);

    dl->AddLine(ImVec2(wPos.x + 12, wPos.y + ImGui::GetFrameHeight() + 4), ImVec2(wPos.x + wSize.x - 12, wPos.y + ImGui::GetFrameHeight() + 4), IM_COL32(255, 255, 255, 10), 1.f);
    
    const char* tabs[] = { S("AIMBOT"), S("VISUALS"), S("MISC"), S("CONFIG") };
    int tabN = 4;
    float tabY = wPos.y + ImGui::GetFrameHeight() + 10;
    float tabH = 28.f;
    float tabW = (wSize.x - 24.f) / tabN;
    float tabX = wPos.x + 12.f;

    float glowW = tabW * 0.5f;
    float glowX = tabX + tabAnim * tabW + (tabW - glowW) * 0.5f;
    dl->AddRectFilled(ImVec2(glowX, tabY + tabH - 2.f), ImVec2(glowX + glowW, tabY + tabH), IM_COL32(0, 200, 255, 255), 1.f);

    for (int i = 0; i < tabN; i++) {
        ImVec2 tMin(tabX + i * tabW, tabY);
        ImVec2 tMax(tabX + (i + 1) * tabW, tabY + tabH);
        ImGui::SetCursorScreenPos(tMin);
        ImGui::InvisibleButton(tabs[i], ImVec2(tabW, tabH));
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) tab = i;
        bool hover = ImGui::IsItemHovered();
        if (hover) dl->AddRectFilled(tMin, tMax, IM_COL32(255, 255, 255, 6), 4.f);
        ImVec2 ts = ImGui::CalcTextSize(tabs[i]);
        dl->AddText(ImVec2(tMin.x + (tabW - ts.x) * 0.5f, tMin.y + (tabH - ts.y) * 0.5f),
            i == tab ? IM_COL32(0, 200, 255, 255) : (hover ? IM_COL32(190, 190, 200, 255) : IM_COL32(120, 120, 140, 200)), tabs[i]);
    }

    ImGui::SetCursorScreenPos(ImVec2(wPos.x + 12, tabY + tabH + 8));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
    float contentH = wPos.y + wSize.y - (tabY + tabH + 8) - 12;
    ImGui::BeginChild(S("Content"), ImVec2(wSize.x - 24, contentH), false, ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.86f, 0.86f, 0.9f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.2f, 0.2f, 0.3f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.08f, 0.15f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.25f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.3f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.15f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.12f, 0.12f, 0.22f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.28f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.f, 0.78f, 1.f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.f, 0.78f, 1.f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.1f, 0.1f, 0.2f, 1.f));

        if (tab == 0) {
            ImGui::Columns(2, 0, false);
            ImGui::TextUnformatted(S("AIMBOT")); ImGui::Separator();
            Toggle(S("Enable Aimbot"), &g_aimbot);
            if (g_aimbot) {
                ImGui::SliderFloat(S("FOV"), &g_aimFov, 0.f, 500.f, S("%.0f"));
                ImGui::Combo(S("Bone"), &g_aimBone, S("Head\0Neck\0Chest\0"));
                Toggle(S("Tracking"), &g_tracking);
                if (g_tracking) ImGui::SliderFloat(S("Speed"), &g_trackingSpeed, 0.1f, 4.f, S("%.1f"));
                if (g_waitingForKey) ImGui::TextUnformatted(S("Press any key..."));
                else {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - 100);
                    if (ImGui::Button(vk_to_str(g_aimKey), ImVec2(80, 22))) g_waitingForKey = true;
                    ImGui::SameLine(); ImGui::TextUnformatted(S("Key"));
                }
            }
            ImGui::NextColumn();
            ImGui::TextUnformatted(S("FLICKBOT")); ImGui::Separator();
            Toggle(S("Enable Flickbot"), &g_flickbot);
            if (g_flickbot) {
                ImGui::SliderFloat(S("FOV"), &g_flickFov, 0.f, 500.f, S("%.0f"));
                ImGui::SliderFloat(S("Speed"), &g_flickSpeed, 0.1f, 8.f, S("%.1f"));
                Toggle(S("ReFlick"), &g_reflick);
                if (g_reflick) ImGui::SliderInt(S("Interval (ms)"), &g_reflickInterval, 0, 350);
                if (g_waitingFlickKey) ImGui::TextUnformatted(S("Press any key..."));
                else {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - 100);
                    if (ImGui::Button(vk_to_str(g_flickKey), ImVec2(80, 22))) g_waitingFlickKey = true;
                    ImGui::SameLine(); ImGui::TextUnformatted(S("Key"));
                }
            }
            ImGui::Columns(1);
        }
        else if (tab == 1) {
            ImGui::Columns(2, 0, false);
            ImGui::TextUnformatted(S("ESP")); ImGui::Separator();
            Toggle(S("2D Box"), &g_drawBoxes);
            Toggle(S("3D Box"), &g_draw3dBox);
            Toggle(S("Skeleton"), &g_drawSkeleton);
            Toggle(S("Snap Lines"), &g_drawLines);
            Toggle(S("Team Check"), &g_teamCheck);
            Toggle(S("Vis Check"), &g_visCheck);
            ImGui::NextColumn();
            ImGui::TextUnformatted(S("OUTLINE & GLOW")); ImGui::Separator();
            Toggle(S("Outline"), &g_outlineEnabled);
            if (g_outlineEnabled) {
                ImGui::SameLine(); Toggle(S("Rainbow"), &g_outlineRainbow);
                if (!g_outlineRainbow) { ImGui::ColorEdit4(S("##ol"), (float*)&g_outlineColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); ImGui::SameLine(); ImGui::TextUnformatted(S("Color")); }
            }
            Toggle(S("Glow"), &g_glowEnabled);
            if (g_glowEnabled) {
                ImGui::ColorEdit4(S("##glv"), (float*)&g_glowColorVisible, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); ImGui::SameLine(); ImGui::TextUnformatted(S("Visible"));
                ImGui::ColorEdit4(S("##gle"), (float*)&g_glowColorEnemy, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); ImGui::SameLine(); ImGui::TextUnformatted(S("Enemy"));
            }
            ImGui::TextUnformatted(S("ULTIMATE")); ImGui::Separator();
            Toggle(S("Ult Bars"), &g_ultBars);
            Toggle(S("Ult Panel"), &g_ultPanel);
            ImGui::Columns(1);
        }
        else if (tab == 2) {
            ImGui::Columns(2, 0, false);
            ImGui::TextUnformatted(S("TRIGGERBOT")); ImGui::Separator();
            Toggle(S("Enable Trigger"), &g_triggerEnabled);
            if (g_triggerEnabled) {
                ImGui::Combo(S("Button"), &g_triggerButton, S("Left\0Right\0"));
                ImGui::SliderFloat(S("Radius"), &g_triggerRadius, 1.f, 200.f, S("%.0f"));
                ImGui::SliderInt(S("Delay (ms)"), &g_triggerDelayMs, 0, 500);
                ImGui::SliderInt(S("Grace (ms)"), &g_triggerGrace, 0, 300);
                Toggle(S("Hold Key"), &g_triggerHoldKey);
                if (g_triggerHoldKey) {
                    if (g_waitTrigKey) ImGui::TextUnformatted(S("Press any key..."));
                    else {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - 100);
                        if (ImGui::Button(vk_to_str(g_triggerKey), ImVec2(80, 22))) g_waitTrigKey = true;
                        ImGui::SameLine(); ImGui::TextUnformatted(S("Key"));
                    }
                }
            }
            ImGui::Columns(1);
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextUnformatted(S("OTHER")); ImGui::Separator();
            Toggle(S("No Recoil"), &g_noRecoil);
        }
        else if (tab == 3) {
            ImGui::TextUnformatted(S("CONFIGURATION")); ImGui::Separator();
            ImGui::TextWrapped(S("Config is saved/loaded automatically every 5 seconds."));
            ImGui::Dummy(ImVec2(0, 10));
            if (ImGui::Button(S("Save Config"), ImVec2(140, 28))) CfgSav();
            ImGui::SameLine(0, 12);
            if (ImGui::Button(S("Load Config"), ImVec2(140, 28))) CfgLd();
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::TextUnformatted(S("INSERT - Toggle Menu"));
        }

    ImGui::PopStyleColor(11);
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::End();
    ImGui::PopStyleVar(6);
    ImGui::PopStyleColor(5);
}

void UpdateESPLogic() {
    if (g_waitTrigKey) {
        for (int k = 1; k < 256; k++) {
            if (GetAsyncKeyState(k) & 0x8000) {
                if (g_waitTrigKey) { g_triggerKey = k; g_waitTrigKey = false; }
                break;
            }
        }
    }

    ApplyOutlines();
    ApplyEngineGlow();
    Trigger();
}

void UpdateScreenPositions() {
    for (auto& e : g_entities) {
        e.screenValid = false;
        e.boxValid    = false;
        if (e.isLocal) continue;

        float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
        bool anyVis = false;

        if (e.boneCount > 0) {
            for (int bi = 0; bi < e.boneCount; bi++) {
                float sx, sy;
                if (WorldToScreen(e.bones[bi], sx, sy)) {
                    e.boneScreen[bi] = { sx, sy };
                    if (sx < minX) minX = sx;
                    if (sy < minY) minY = sy;
                    if (sx > maxX) maxX = sx;
                    if (sy > maxY) maxY = sy;
                    anyVis = true;
                } else {
                    e.boneScreen[bi] = { -1.f, -1.f };
                }
            }
        } else {
            float sx1, sy1, sx2, sy2;
            float3 head = { e.pos.x, e.pos.y, e.pos.z + 1.8f };
            if (WorldToScreen(e.pos, sx1, sy1) && WorldToScreen(head, sx2, sy2)) {
                float w = fabsf(sy1 - sy2) * 0.45f;
                minX = sx1 - w; maxX = sx1 + w;
                minY = sy2;     maxY = sy1;
                anyVis = true;
            }
        }

        if (!anyVis) continue;
        e.screenValid = true;
        e.minX = minX; e.minY = minY; e.maxX = maxX; e.maxY = maxY;

        {
            float3 bp = e.hasBones ? e.bones[g_aimBone == 0 ? 0 : 2] : (e.boneCount > 0 ? e.bones[0] : e.pos);
            float sx1, sy1, sx2, sy2;
            if (WorldToScreen({bp.x, bp.y + 0.1f, bp.z}, sx1, sy1) &&
                WorldToScreen({bp.x, bp.y - 1.8f, bp.z}, sx2, sy2)) {
                float h       = fabsf(sy1 - sy2);
                float w       = h * 0.85f;
                e.boxT        = sy1 - (h / 5.0f) - 1.0f;
                e.boxB        = e.boxT + h + 12.0f;
                e.boxL        = sx1 - (w / 2.0f) - 1.0f;
                e.boxR        = e.boxL + w + 2.0f;
                e.boxValid    = true;
            }
        }
    }
}

void DrawESP() {
    auto* dl = ImGui::GetForegroundDrawList();

    if (!(g_drawBoxes || g_drawSkeleton || g_drawLines || g_draw3dBox)) {
        DrawUltBars(dl); DrawUltPanel(dl);
        return;
    }

    for (auto& e : g_entities) {
        if (e.isLocal) continue;
        if (g_teamCheck && g_localTeam != 0 && e.team == g_localTeam) continue;
        if (!e.screenValid) continue;

        if (g_drawSkeleton && e.hasBones) {
            for (int li = 0; li < 17; li++) {
                int i1 = g_boneLines[li][0], i2 = g_boneLines[li][1];
                if (i1 >= e.boneCount || i2 >= e.boneCount) continue;
                if (e.boneScreen[i1].x < 0 || e.boneScreen[i2].x < 0) continue;
                if ((e.bones[i1].x==0&&e.bones[i1].y==0&&e.bones[i1].z==0)||
                    (e.bones[i2].x==0&&e.bones[i2].y==0&&e.bones[i2].z==0)) continue;
                dl->AddLine(ImVec2(e.boneScreen[i1].x, e.boneScreen[i1].y),
                            ImVec2(e.boneScreen[i2].x, e.boneScreen[i2].y), IM_COL32(0,0,0,160), 3.0f);
                dl->AddLine(ImVec2(e.boneScreen[i1].x, e.boneScreen[i1].y),
                            ImVec2(e.boneScreen[i2].x, e.boneScreen[i2].y), IM_COL32(255,255,255,255), 1.8f);
            }
        }

        if (g_drawBoxes && e.boxValid) {
            float lft = e.boxL, top = e.boxT, rgt = e.boxR, bot = e.boxB;
            float cw  = (rgt - lft) * 0.20f;
            float ch  = (bot - top) * 0.20f;

            auto Corner = [&](float x1, float y1, float dx, float dy) {
                dl->AddLine(ImVec2(x1, y1), ImVec2(x1+dx, y1),    IM_COL32(0,0,0,200),       3.0f);
                dl->AddLine(ImVec2(x1, y1), ImVec2(x1, y1+dy),    IM_COL32(0,0,0,200),       3.0f);
                dl->AddLine(ImVec2(x1, y1), ImVec2(x1+dx, y1),    IM_COL32(255,255,255,255), 1.5f);
                dl->AddLine(ImVec2(x1, y1), ImVec2(x1, y1+dy),    IM_COL32(255,255,255,255), 1.5f);
            };
            Corner(lft, top, +cw, +ch);
            Corner(rgt, top, -cw, +ch);
            Corner(lft, bot, +cw, -ch);
            Corner(rgt, bot, -cw, -ch);
        }

        if (g_drawLines) {
            float sx = (e.boxValid) ? (e.boxL + e.boxR) * 0.5f : e.boneScreen[0].x;
            float sy = (e.boxValid) ? e.boxB                     : e.boneScreen[0].y;
            if (sx >= 0)
                dl->AddLine(ImVec2(g_screenW/2, g_screenH/2), ImVec2(sx, sy), IM_COL32(255,255,255,200), 1.0f);
        }

        if (g_draw3dBox && e.hasBones) {
            float3 corners[8];
            if (get_3dbox_corners(e.pBoneData, e.pos, e.rot_y, corners)) {
                float2 sc[8]; bool allVis = true;
                for (int i = 0; i < 8; i++) {
                    if (!WorldToScreen(corners[i], sc[i].x, sc[i].y)) { allVis = false; break; }
                }
                if (allVis) {
                    int edges[12][2] = {{0,1},{1,3},{3,2},{2,0},{4,5},{5,7},{7,6},{6,4},{0,4},{1,5},{2,6},{3,7}};
                    for (int i = 0; i < 12; i++)
                        dl->AddLine(ImVec2(sc[edges[i][0]].x, sc[edges[i][0]].y),
                                    ImVec2(sc[edges[i][1]].x, sc[edges[i][1]].y), IM_COL32(255,255,255,255), 1.0f);
                }
            }
        }
    }
    DrawUltBars(dl);
    DrawUltPanel(dl);
}