#pragma once
#include "sdk.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

static inline void _cfgp(char* buf, int n)
{
	char	app[MAX_PATH];

	GetEnvironmentVariableA(S("APPDATA"), app, MAX_PATH);
	snprintf(buf, n, S("%s\\fkyoubitch.ini"), app);
}

static inline void CfgSav()
{
	char	p[MAX_PATH];
	FILE*	f;

	_cfgp(p, MAX_PATH);
	f = fopen(p, S("w"));
	if (!f) return;

	fprintf(f, S("[ESP]\nboxes=%d\nskel=%d\nlines=%d\nbox3d=%d\nteam=%d\nvis=%d\n"),
		(int)g_drawBoxes,(int)g_drawSkeleton,(int)g_drawLines,(int)g_draw3dBox,(int)g_teamCheck,(int)g_visCheck);
	fprintf(f, S("outline=%d\nrainbow=%d\nglow=%d\nultbars=%d\nultpanel=%d\n"),(int)g_outlineEnabled,(int)g_outlineRainbow,(int)g_glowEnabled,(int)g_ultBars,(int)g_ultPanel);
	fprintf(f, S("olcol=%.3f,%.3f,%.3f,%.3f\nglvis=%.3f,%.3f,%.3f,%.3f\nglen=%.3f,%.3f,%.3f,%.3f\n"),
		g_outlineColor.x,g_outlineColor.y,g_outlineColor.z,g_outlineColor.w,
		g_glowColorVisible.x,g_glowColorVisible.y,g_glowColorVisible.z,g_glowColorVisible.w,
		g_glowColorEnemy.x,g_glowColorEnemy.y,g_glowColorEnemy.z,g_glowColorEnemy.w);

	fprintf(f, S("[AIM]\naim=%d\ntrack=%d\nflick=%d\nreflick=%d\n"),(int)g_aimbot,(int)g_tracking,(int)g_flickbot,(int)g_reflick);
	fprintf(f, S("aimfov=%.2f\nffov=%.2f\nfspd=%.2f\ntspd=%.2f\nrint=%d\nakey=%d\nfkey=%d\nabone=%d\n"),
		g_aimFov,g_flickFov,g_flickSpeed,g_trackingSpeed,g_reflickInterval,g_aimKey,g_flickKey,g_aimBone);

	fprintf(f, S("[TRIG]\ntrig=%d\ntbtn=%d\ntrad=%.2f\ntdms=%d\ntgms=%d\nthold=%d\ntkey=%d\n"),
		(int)g_triggerEnabled,g_triggerButton,g_triggerRadius,g_triggerDelayMs,g_triggerGrace,(int)g_triggerHoldKey,g_triggerKey);

	fclose(f);
}

static inline void _pv4(const char* v, ImVec4& c)
{
	sscanf(v, S("%f,%f,%f,%f"), &c.x, &c.y, &c.z, &c.w);
}

static inline void _ln(const char* k, const char* v, int s)
{
#define C(k) !strcmp(k,S(k))
#define ATOI(v) atoi(v)
#define ATOF(v) (float)atof(v)
	if (s == 0) {
		if      (C("boxes"))   g_drawBoxes      = ATOI(v)!=0;
		else if (C("skel"))    g_drawSkeleton   = ATOI(v)!=0;
		else if (C("lines"))   g_drawLines      = ATOI(v)!=0;
		else if (C("box3d"))   g_draw3dBox      = ATOI(v)!=0;
		else if (C("team"))    g_teamCheck      = ATOI(v)!=0;
		else if (C("vis"))     g_visCheck       = ATOI(v)!=0;
		else if (C("outline")) g_outlineEnabled = ATOI(v)!=0;
		else if (C("rainbow")) g_outlineRainbow = ATOI(v)!=0;
		else if (C("glow"))    g_glowEnabled    = ATOI(v)!=0;
		else if (C("ultbars")) g_ultBars        = ATOI(v)!=0;
		else if (C("ultpanel"))g_ultPanel       = ATOI(v)!=0;
		else if (C("olcol"))   _pv4(v, g_outlineColor);
		else if (C("glvis"))   _pv4(v, g_glowColorVisible);
		else if (C("glen"))    _pv4(v, g_glowColorEnemy);
	} else if (s == 1) {
		if      (C("aim"))     g_aimbot         = ATOI(v)!=0;
		else if (C("track"))   g_tracking       = ATOI(v)!=0;
		else if (C("flick"))   g_flickbot       = ATOI(v)!=0;
		else if (C("reflick")) g_reflick        = ATOI(v)!=0;
		else if (C("aimfov"))  g_aimFov         = ATOF(v);
		else if (C("ffov"))    g_flickFov       = ATOF(v);
		else if (C("fspd"))    g_flickSpeed     = ATOF(v);
		else if (C("tspd"))    g_trackingSpeed  = ATOF(v);
		else if (C("rint"))    g_reflickInterval= ATOI(v);
		else if (C("akey"))    g_aimKey         = ATOI(v);
		else if (C("fkey"))    g_flickKey       = ATOI(v);
		else if (C("abone"))   g_aimBone        = ATOI(v);
	} else if (s == 2) {
		if      (C("trig"))    g_triggerEnabled = ATOI(v)!=0;
		else if (C("tbtn"))    g_triggerButton  = ATOI(v);
		else if (C("trad"))    g_triggerRadius  = ATOF(v);
		else if (C("tdms"))    g_triggerDelayMs = ATOI(v);
		else if (C("tgms"))    g_triggerGrace   = ATOI(v);
		else if (C("thold"))   g_triggerHoldKey = ATOI(v)!=0;
		else if (C("tkey"))    g_triggerKey     = ATOI(v);
	}
#undef C
#undef ATOI
#undef ATOF
}

static inline void CfgLd()
{
	char	p[MAX_PATH];
	char	line[256];
	char*	eq;
	FILE*	f;
	int		s;
	int		n;

	_cfgp(p, MAX_PATH);
	f = fopen(p, S("r"));
	if (!f) return;
	s = -1;
	while (fgets(line, sizeof(line), f)) {
		n = (int)strlen(line);
		while (n > 0 && (line[n-1]=='\n'||line[n-1]=='\r')) line[--n]='\0';
		if (!n || line[0]=='#') continue;
		if (line[0]=='[') {
			if      (!strncmp(line, S("[ESP]"),5))  s=0;
			else if (!strncmp(line, S("[AIM]"),5))  s=1;
			else if (!strncmp(line, S("[TRIG]"),6)) s=2;
			else s=-1;
			continue;
		}
		eq = strchr(line, '=');
		if (!eq || s < 0) continue;
		*eq = '\0';
		_ln(line, eq+1, s);
	}
	fclose(f);
}
