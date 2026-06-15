#include "sdk.hpp"
#include "offsets.hpp"
#include "utils.hpp"
#include "imgui.h"
#include <unordered_map>
#include <cmath>
#include <intrin.h>

inline uint64_t _outlineBabe(uint64_t f)
{
	uint64_t	kptr;
	uint64_t	gkey;
	uint64_t	bkey;
	uint64_t	r;

	kptr = SDK->RPM<uint64_t>(SDK->dwGameBase + offset::GlobalKeyPtr);
	gkey = SDK->RPM<uint64_t>(kptr + offset::outline_offsets);
	bkey = (uint64_t)SDK->RPM<uint8_t>(SDK->dwGameBase + offset::bytekey_outline);

	auto e = [&](uint64_t f) -> uint64_t
	{
		r = f;
		r = _rotl64(r, 23);
		r ^= 0xEF781B6466FAB59BULL;
		r ^= static_cast<uint64_t>(bkey);
		r -= 0x2240EA534C11100DULL;
		r += 0x605EC85DF1D6897DULL;
		r ^= gkey;
		r += 0x1F21DE5151741226ULL;
		return (r);
	};
	return (e(static_cast<uint64_t>(f)));
}

static inline uint32_t _rainbowHoe(float spd)
{
	float	h;
	float	f;
	float	r;
	float	g;
	float	b;
	int		i;

	h = fmodf(GetTickCount64() * 0.001f * spd, 1.0f);
	i = (int)(h * 6);
	f = h * 6.0f - i;
	r = g = b = 0.0f;
	switch (i % 6) {
		case 0: r=1;   g=f;   b=0;   break;
		case 1: r=1-f; g=1;   b=0;   break;
		case 2: r=0;   g=1;   b=f;   break;
		case 3: r=0;   g=1-f; b=1;   break;
		case 4: r=f;   g=0;   b=1;   break;
		default:r=1;   g=0;   b=1-f; break;
	}
	return IM_COL32((int)(r*255),(int)(g*255),(int)(b*255),255);
}

inline uint64_t _entryBabe(uint64_t base)
{
	uint64_t	a1;
	int			cnt;
	uint64_t	arr;

	if (!base) return 0;
	a1  = base + 0x20;
	cnt = (int)SDK->RPM<uint32_t>(a1 + 0x68);
	arr = SDK->RPM<uint64_t>(a1 + 0x60);
	if (cnt <= 0 || cnt > 64) return 0;
	if (arr < 0x100000ULL || arr >= 0x800000000000ULL) return 0;
	return arr + (uint64_t)(cnt - 1) * 0x20;
}

inline void _slapOutline(uint64_t base, uint32_t type, uint32_t col)
{
	uint64_t	entry;
	uint64_t	enc;

	if (!base) return;
	entry = _entryBabe(base);
	if (!entry) return;
	enc = _outlineBabe((uint64_t)type);
	SDK->WPM<uint64_t>(entry + 0x10, enc);
	SDK->WPM<uint64_t>(entry + 0x08, enc);
	if (type && col) {
		SDK->WPM<uint32_t>(base + 0x110, col);
		SDK->WPM<uint32_t>(base + 0x124, col);
		SDK->WPM<uint8_t> (base + 0x114, 1);
		SDK->WPM<float>   (base + 0x130, 1.0f);
	} else {
		SDK->WPM<uint32_t>(base + 0x110, 0);
		SDK->WPM<uint32_t>(base + 0x124, 0);
		SDK->WPM<uint8_t> (base + 0x114, 0);
		SDK->WPM<float>   (base + 0x130, 0.0f);
	}
}

inline void ApplyOutlines()
{
	struct OC { uint64_t base; uint32_t last; };
	static std::unordered_map<uint64_t, OC> s_c;
	static int s_skip;
	static int s_cd;
	uint32_t	col;
	uint32_t	type;
	uint64_t	ob;
	bool		isN;
	bool		doW;
	bool		f;

	if (!g_outlineEnabled) {
		if (s_cd > 0) {
			s_cd--;
			if (s_cd % 5 == 0) {
				for (auto& e : g_entities) {
					if (e.isLocal) continue;
					auto it = s_c.find(e.addr);
					ob = (it != s_c.end()) ? it->second.base : GetDecryptedComponent(e.common, TYPE_OUTLINE);
					if (ob) { _slapOutline(ob, 0, 0); s_c.erase(e.addr); }
				}
			}
		} else s_c.clear();
		return;
	}

	s_cd = 120;
	doW  = (++s_skip % 10 == 0);
	col  = g_outlineRainbow ? _rainbowHoe(0.25f) : ImGui::ColorConvertFloat4ToU32(g_outlineColor);

	for (auto& e : g_entities) {
		if (e.isLocal) continue;
		if (g_teamCheck && g_localTeam && e.team == g_localTeam) continue;

		type = e.visible ? 0x1 : 0x2;
		isN  = (s_c.find(e.addr) == s_c.end());

		if (isN) {
			ob = GetDecryptedComponent(e.common, TYPE_OUTLINE);
			if (!ob) continue;
			s_c[e.addr] = { ob, 0xFFFFFFFF };
		} else ob = s_c[e.addr].base;
		if (!ob) continue;

		if (type != s_c[e.addr].last || doW || g_outlineRainbow) {
			_slapOutline(ob, type, col);
			s_c[e.addr].last = type;
		}
	}

	if (doW) {
		for (auto it = s_c.begin(); it != s_c.end(); ) {
			f = false;
			for (const auto& e : g_entities) if (e.addr == it->first) { f = true; break; }
			it = f ? std::next(it) : s_c.erase(it);
		}
	}
}

inline void ApplyEngineGlow()
{
	uint32_t	cv;
	uint32_t	co;
	uint64_t	ob;

	if (!g_glowEnabled) return;
	cv = ImGui::ColorConvertFloat4ToU32(g_glowColorVisible);
	co = ImGui::ColorConvertFloat4ToU32(g_glowColorEnemy);
	for (auto& e : g_entities) {
		if (e.isLocal) continue;
		if (g_teamCheck && g_localTeam && e.team == g_localTeam) continue;
		ob = GetDecryptedComponent(e.common, TYPE_OUTLINE);
		if (!ob) continue;
		_slapOutline(ob, e.visible ? 0x1 : 0x2, e.visible ? cv : co);
	}
}
