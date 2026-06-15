#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>
#include <windows.h>
#include <unordered_map>
#include "imgui.h"
#include "stringencryption.hpp"
#include "spoof_call.hpp"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;

struct float3 { float x, y, z; };
struct float2 { float x, y; };

struct Matrix {
    float data[16];
    float& at(int r, int c) { return data[r * 4 + c]; }
};

struct EntityComponentCache {
    uint64_t link = 0;
    bool link_cached = false;
    uint64_t health = 0;
    bool health_cached = false;
    uint64_t velocity = 0;
    bool velocity_cached = false;
    uint64_t rotation = 0;
    bool rotation_cached = false;
    uint64_t pc = 0;
    bool pc_cached = false;
    uint64_t team = 0;
    bool team_cached = false;
    uint64_t visibility = 0;
    bool visibility_cached = false;
    uint64_t hc2 = 0;
    bool hc2_cached = false;
};
extern std::unordered_map<uint64_t, EntityComponentCache> g_componentCache;

struct EntityInfo {
    uint64_t addr;
    uint64_t common;
    uint16_t heroId;
    uint32_t uid;
    float health, health_max;
    float3 pos;
    float rot_y;
    float3 bones[18];
    float2 boneScreen[18];
    int boneCount;
    bool hasBones;
    bool isLocal;
    bool visible;
    bool hasPC;
    int team;
    uint64_t pBoneData;

    bool  screenValid;
    bool  boxValid;
    float boxL, boxT, boxR, boxB;
    float minX, minY, maxX, maxY;
};

struct OverwatchSDK {
    HANDLE hProcess = nullptr;
    uint64_t dwGameBase = 0;
    template<typename T> T RPM(uint64_t addr) {
        T val{}; ReadProcessMemory(hProcess, (LPCVOID)addr, &val, sizeof(T), nullptr); return val;
    }
    template<typename T> void WPM(uint64_t addr, T val) {
        WriteProcessMemory(hProcess, (LPVOID)addr, &val, sizeof(T), nullptr);
    }
};

struct EntPair { uint64_t e; uint64_t pad; };

extern OverwatchSDK* SDK;
extern std::vector<EntityInfo> g_entities;
extern Matrix g_viewMatrix;
extern float3 g_cameraPos;
extern float g_screenW, g_screenH;
extern DWORD g_pid;
extern uint64_t g_base;
extern HWND g_hWnd;
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRT;
extern bool g_running, g_menuOpen;
extern bool g_drawBoxes, g_drawSkeleton, g_drawLines, g_draw3dBox, g_teamCheck, g_visCheck;
extern bool g_noRecoil;
extern HANDLE g_recoilThread;
extern bool g_aimbot, g_tracking, g_flickbot, g_reflick;
extern float g_trackingSpeed, g_aimFov, g_flickFov, g_flickSpeed;
extern int g_reflickInterval, g_aimKey, g_flickKey, g_aimBone, g_localTeam;
extern bool g_waitingForKey, g_waitingFlickKey;
extern bool g_outlineEnabled, g_outlineRainbow, g_glowEnabled;
extern ImVec4 g_outlineColor, g_glowColorVisible, g_glowColorEnemy;
extern bool g_triggerEnabled, g_triggerHoldKey;
extern int  g_triggerButton, g_triggerDelayMs, g_triggerGrace, g_triggerKey;
extern float g_triggerRadius;
extern bool g_waitTrigKey;
extern bool g_ultBars, g_ultPanel;