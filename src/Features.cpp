#include "features.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "offsets.hpp"

void DoLocalPlayerDetection() {
    bool foundLocal = false;
    for (auto& e : g_entities) {
        if (e.isLocal) { foundLocal = true; break; }
    }
    if (!foundLocal) {
        float bestDist = 1e9f;
        for (auto& e : g_entities) {
            if (e.boneCount <= 0) continue;
            float dx = e.bones[0].x - g_cameraPos.x;
            float dy = e.bones[0].y - g_cameraPos.y;
            float dz = e.bones[0].z - g_cameraPos.z;
            float d = dx*dx + dy*dy + dz*dz;
            if (d < bestDist) bestDist = d;
        }
        if (bestDist < 25.0f) {
            for (auto& e : g_entities) {
                if (e.boneCount <= 0) continue;
                float dx = e.bones[0].x - g_cameraPos.x;
                float dy = e.bones[0].y - g_cameraPos.y;
                float dz = e.bones[0].z - g_cameraPos.z;
                if (fabsf(dx*dx + dy*dy + dz*dz - bestDist) < 0.01f) {
                    e.isLocal = true; break;
                }
            }
        }
    }
    g_localTeam = 0;
    for (auto& e : g_entities) {
        if (e.isLocal) { g_localTeam = e.team; break; }
    }
}

void DoAimbot() {
    if (!g_aimbot) return;
    uint64_t localPC = 0;
    float3 targetPos = {};
    bool haveTarget = false;
    float bestScreenDist = 1e9f;
    for (auto& e : g_entities) {
        if (!e.isLocal && e.boneCount > 0 && e.health > 0.f && (!g_visCheck || e.visible) && (!g_teamCheck || g_localTeam == 0 || e.team != g_localTeam)) {
            float sx, sy;
            int boneIdx = min(g_aimBone, e.boneCount - 1);
            if (WorldToScreen(e.bones[boneIdx], sx, sy)) {
                float d = fabsf(sx - g_screenW/2) + fabsf(sy - g_screenH/2);
                float fovPixels = g_aimFov * (g_screenW / 1920.0f);
                if (d < bestScreenDist && d < fovPixels) {
                    bestScreenDist = d;
                    targetPos = e.bones[boneIdx];
                    haveTarget = true;
                }
            }
        } else if (e.isLocal) {
            localPC = GetDecryptedComponent(e.addr, TYPE_PLAYERCONTROLLER);
        }
    }
    if (haveTarget && localPC && (GetAsyncKeyState(g_aimKey) & 0x8000)) {
        float dx = targetPos.x - g_cameraPos.x;
        float dy = targetPos.y - g_cameraPos.y;
        float dz = targetPos.z - g_cameraPos.z;
        float dist = sqrtf(dx*dx + dy*dy + dz*dz);
        if (dist > 0.01f) {
            float3 dir = { dx/dist, dy/dist, dz/dist };
            if (g_tracking) {
                float3 cur = SDK->RPM<float3>(localPC + 0x1260);
                float speed = g_trackingSpeed * 0.1f;
                cur.x += (dir.x - cur.x) * speed;
                cur.y += (dir.y - cur.y) * speed;
                cur.z += (dir.z - cur.z) * speed;
                SDK->WPM(localPC + 0x1260, cur);
            } else {
                SDK->WPM(localPC + 0x1260, dir);
            }
        }
    }
}

void DoFlickbot() {
    if (!g_flickbot) return;
    uint64_t localPC = 0;
    float3 targetPos = {};
    bool haveTarget = false;
    float bestScreenDist = 1e9f;
    for (auto& e : g_entities) {
        if (!e.isLocal && e.boneCount > 0 && e.health > 0.f && (!g_visCheck || e.visible) && (!g_teamCheck || g_localTeam == 0 || e.team != g_localTeam)) {
            float sx, sy;
            int boneIdx = min(g_aimBone, e.boneCount - 1);
            if (WorldToScreen(e.bones[boneIdx], sx, sy)) {
                float d = fabsf(sx - g_screenW/2) + fabsf(sy - g_screenH/2);
                float fovPixels = g_flickFov * (g_screenW / 1920.0f);
                if (d < bestScreenDist && d < fovPixels) {
                    bestScreenDist = d;
                    targetPos = e.bones[boneIdx];
                    haveTarget = true;
                }
            }
        } else if (e.isLocal) {
            localPC = GetDecryptedComponent(e.addr, TYPE_PLAYERCONTROLLER);
        }
    }
    if (haveTarget && localPC && (GetAsyncKeyState(g_flickKey) & 0x8000)) {
        float dx = targetPos.x - g_cameraPos.x;
        float dy = targetPos.y - g_cameraPos.y;
        float dz = targetPos.z - g_cameraPos.z;
        float dist = sqrtf(dx*dx + dy*dy + dz*dz);
        if (dist > 0.01f) {
            float3 dir = { dx/dist, dy/dist, dz/dist };
            float3 cur = SDK->RPM<float3>(localPC + 0x1260);
            float speed = g_flickSpeed * 0.15f;
            cur.x += (dir.x - cur.x) * speed;
            cur.y += (dir.y - cur.y) * speed;
            cur.z += (dir.z - cur.z) * speed;
            SDK->WPM(localPC + 0x1260, cur);
            float err = fabsf(cur.x - dir.x) + fabsf(cur.y - dir.y) + fabsf(cur.z - dir.z);
            static bool flicked = false;
            static DWORD lastFlickTime = 0;
            if (err < 0.05f) {
                if (!flicked) {
                    flicked = true; lastFlickTime = GetTickCount();
                    INPUT input = {}; input.type = INPUT_MOUSE; input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; SendInput(1, &input, sizeof(INPUT));
                    input.mi.dwFlags = MOUSEEVENTF_LEFTUP; SendInput(1, &input, sizeof(INPUT));
                } else if (g_reflick && GetTickCount() - lastFlickTime >= (DWORD)g_reflickInterval) {
                    lastFlickTime = GetTickCount();
                    INPUT input = {}; input.type = INPUT_MOUSE; input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; SendInput(1, &input, sizeof(INPUT));
                    input.mi.dwFlags = MOUSEEVENTF_LEFTUP; SendInput(1, &input, sizeof(INPUT));
                }
            } else flicked = false;
        }
    }
}

DWORD WINAPI NoRecoilThread(LPVOID) {
    while (g_running) {
        if (g_noRecoil) {
            uint64_t pc = 0;
            for (const auto& e : g_entities) {
                if (e.isLocal) {
                    pc = GetDecryptedComponent(e.addr, TYPE_PLAYERCONTROLLER);
                    if (!pc) pc = GetDecryptedComponent(e.common, TYPE_PLAYERCONTROLLER);
                    break;
                }
            }
            if (pc) {
                float val = 1.0f;
                SDK->WPM(pc + 0x16C4, val);
            }
        }
        Sleep(5);
    }
    return 0;
}
