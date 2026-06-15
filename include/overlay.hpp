#pragma once
#include "sdk.hpp"

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND MakeOverlay();
bool InitD3D(HWND hWnd);
void CreateRT();
void CleanupD3D();