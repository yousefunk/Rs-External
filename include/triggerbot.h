#include "sdk.hpp"
#include "utils.hpp"
#include <cmath>

inline void Trigger()
{
	static bool		s_dn;
	static int		s_off;
	static DWORD	s_up;
	static DWORD	s_fire;

	bool	keyOk;
	bool	inZ;
	bool	ok;
	float	cx;
	float	cy;
	float	sx;
	float	sy;
	float	dx;
	float	dy;
	DWORD	now;

	if (!g_triggerEnabled) {
		if (s_dn) { mouse_event(g_triggerButton ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP, 0,0,0,0); s_dn = false; }
		return;
	}
	keyOk = !g_triggerHoldKey || (GetAsyncKeyState(g_triggerKey) & 0x8000);
	if (!keyOk) {
		if (s_dn) { mouse_event(g_triggerButton ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP, 0,0,0,0); s_dn = false; }
		return;
	}

	cx  = g_screenW * 0.5f;
	cy  = g_screenH * 0.5f;
	now = GetTickCount();
	inZ = false;

	for (auto& e : g_entities) {
		if (e.isLocal) continue;
		if (g_teamCheck && g_localTeam && e.team == g_localTeam) continue;
		if (g_visCheck && !e.visible) continue;
		if (e.boneCount > 0) {
			if (!WorldToScreen(e.bones[0], sx, sy)) continue;
		} else {
			if (!WorldToScreen(e.pos, sx, sy)) continue;
		}
		dx = sx - cx; dy = sy - cy;
		if (sqrtf(dx*dx + dy*dy) <= g_triggerRadius) { inZ = true; break; }
	}

	if (!inZ) {
		if (s_dn) {
			if (++s_off >= 5) {
				mouse_event(g_triggerButton ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP, 0,0,0,0);
				s_dn = false; s_off = 0; s_up = now;
			}
		} else { s_off = 0; }
	} else {
		s_off = 0;
		if (!s_dn) {
			ok = (!s_up || (now - s_up) >= (DWORD)g_triggerGrace)
			  && (now - s_fire) >= (DWORD)g_triggerDelayMs;
			if (ok) { mouse_event(g_triggerButton ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_LEFTDOWN, 0,0,0,0); s_dn = true; s_fire = now; }
		}
	}
}
