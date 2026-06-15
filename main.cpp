#define IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "include/config.h"
#include <d3d11.h>
#include <dxgi.h>
#include <tlhelp32.h>
#include <psapi.h>
#include "include/overlay.hpp"
#include "include/render.hpp"
#include "include/features.hpp"
#include "include/memory.hpp"
#include "include/utils.hpp"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "psapi.lib")

int main() {
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    timeBeginPeriod(1);
    HANDLE _hC = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(_hC, FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN);
    printf(S("RsExternal made by nohtinginLife and snowancestor\n"));
    SetConsoleTextAttribute(_hC, FOREGROUND_INTENSITY | FOREGROUND_GREEN);
    while (!g_pid) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(snap, &pe)) do {
            { char _en[64]; wcstombs(_en, pe.szExeFile, 63); if (_stricmp(_en, S("Overwatch.exe")) == 0) { g_pid = pe.th32ProcessID; break; } }
        } while (Process32NextW(snap, &pe));
        CloseHandle(snap);
        if (!g_pid) Sleep(1000);
    }
    HANDLE hp = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, g_pid);
    HMODULE hm = 0; DWORD need = 0;
    if (EnumProcessModules(hp, &hm, sizeof(hm), &need)) {
        MODULEINFO mi = {};
        GetModuleInformation(hp, hm, &mi, sizeof(mi));
        g_base = (uint64_t)mi.lpBaseOfDll;
    }
    SetConsoleTextAttribute(_hC, FOREGROUND_INTENSITY | FOREGROUND_GREEN);
    printf(S("Overwatch.exe Found! Pid: %lu  BaseAddress: 0x%llx\n"), g_pid, g_base);

    OverwatchSDK sdk;
    sdk.hProcess = hp;
    sdk.dwGameBase = g_base;
    SDK = &sdk;

    ReadEntities();
    g_hWnd = MakeOverlay();
    if (!g_hWnd) { printf(S("[-] Overlay failed\n")); CloseHandle(hp); return 1; }
    if (!InitD3D(g_hWnd)) { printf(S("[-] D3D failed\n")); DestroyWindow(g_hWnd); CloseHandle(hp); return 1; }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    { static char _p[MAX_PATH]; snprintf(_p, MAX_PATH, S("%s\\RsExternal.ini"), S("C:\\ProgramData")); io.IniFilename = _p; }
    if (!io.Fonts->AddFontFromFileTTF(S("C:\\Windows\\Fonts\\segoeuib.ttf"), 16.f))
        io.Fonts->AddFontFromFileTTF(S("C:\\Windows\\Fonts\\segoeui.ttf"), 16.f);
    io.Fonts->Build();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);
    printf(S("[+] Ready.\n"));
    SetConsoleTextAttribute(_hC, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
    printf(S("Press Insert to Close/Show Menu\n"));
    SetConsoleTextAttribute(_hC, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    printf(S("This is Free Product, if u bought this so u got scammed, released in UC.\n"));
    SetConsoleTextAttribute(_hC, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    printf(S("Source Code Released in my github.\n"));
    SetConsoleTextAttribute(_hC, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    CfgLd();
    g_recoilThread = CreateThread(nullptr, 0, NoRecoilThread, nullptr, 0, nullptr);

    MSG msg;
    HWND  s_gameHwnd = nullptr;
    RECT  s_lastRect = {};
    LARGE_INTEGER s_freq = {}, s_lastTick = {}, s_lastSave = {};
    QueryPerformanceFrequency(&s_freq);
    QueryPerformanceCounter(&s_lastTick);
    s_lastSave = s_lastTick;
    double s_freqMs = s_freq.QuadPart / 1000.0;

    while (g_running) {
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
        if (!g_running) break;

        LARGE_INTEGER qpcNow;
        QueryPerformanceCounter(&qpcNow);

        if (!s_gameHwnd) s_gameHwnd = FindWindowA(S("TankWindowClass"), nullptr);
        if (s_gameHwnd && !g_menuOpen) {
            RECT r;
            GetClientRect(s_gameHwnd, &r);
            if (r.left != s_lastRect.left || r.top != s_lastRect.top || r.right != s_lastRect.right || r.bottom != s_lastRect.bottom) {
                s_lastRect = r;
                POINT pt = {0, 0};
                ClientToScreen(s_gameHwnd, &pt);
                SetWindowPos(g_hWnd, HWND_TOPMOST, pt.x, pt.y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
            }
        }

        if (GetAsyncKeyState(VK_INSERT) & 1) {
            g_menuOpen = !g_menuOpen;
            LONG ex = GetWindowLongA(g_hWnd, GWL_EXSTYLE);
            if (g_menuOpen) {
                ex &= ~WS_EX_TRANSPARENT;
                SetWindowLongA(g_hWnd, GWL_EXSTYLE, ex);
                while (ShowCursor(TRUE) < 0);
            } else {
                ex |= WS_EX_TRANSPARENT;
                SetWindowLongA(g_hWnd, GWL_EXSTYLE, ex);
                while (ShowCursor(FALSE) >= 0);
            }
        }

        if ((double)(qpcNow.QuadPart - s_lastSave.QuadPart) / s_freqMs >= 5000.0) { CfgSav(); s_lastSave = qpcNow; }

        if ((double)(qpcNow.QuadPart - s_lastTick.QuadPart) / s_freqMs >= 7.0) {
            s_lastTick = qpcNow;
            ReadEntities();
            ReadViewMatrix();
            DoLocalPlayerDetection();
            DoAimbot();
            DoFlickbot();
            UpdateESPLogic();
            UpdateScreenPositions();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (g_menuOpen) RenderMenu();
        DrawESP();
        ImGui::Render();

        g_pd3dContext->OMSetRenderTargets(1, &g_mainRT, nullptr);
        float clr[4] = { 0, 0, 0, 0 };
        g_pd3dContext->ClearRenderTargetView(g_mainRT, clr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(0, 0);
        Sleep(1);
    }

    g_running = false;
    CfgSav();
    timeEndPeriod(1);
    if (g_recoilThread) { WaitForSingleObject(g_recoilThread, 1000); CloseHandle(g_recoilThread); }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupD3D();
    DestroyWindow(g_hWnd);
    CloseHandle(hp);
    return 0;
}