#include "sdk.hpp"
#include "utils.hpp"

static inline const char* _hname(uint16_t id)
{
	switch (id) {
		case 0x0002: return S("Reaper");       case 0x0003: return S("Tracer");
		case 0x0004: return S("Mercy");        case 0x0005: return S("Hanzo");
		case 0x0006: return S("Torbjorn");     case 0x0007: return S("Reinhardt");
		case 0x0008: return S("Pharah");       case 0x0009: return S("Winston");
		case 0x000A: return S("Widowmaker");   case 0x0015: return S("Bastion");
		case 0x0016: return S("Symmetra");     case 0x0020: return S("Zenyatta");
		case 0x0029: return S("Genji");        case 0x0040: return S("Roadhog");
		case 0x0042: return S("Cassidy");      case 0x0065: return S("Junkrat");
		case 0x0068: return S("Zarya");        case 0x006E: return S("Soldier:76");
		case 0x0079: return S("Lucio");        case 0x007A: return S("D.Va");
		case 0x00DD: return S("Mei");          case 0x012E: return S("Sombra");
		case 0x012F: return S("Doomfist");     case 0x013B: return S("Ana");
		case 0x013E: return S("Orisa");        case 0x0195: return S("Brigitte");
		case 0x01A2: return S("Moira");        case 0x01CA: return S("Wrecking Ball");
		case 0x01EC: return S("Sojourn");      case 0x0200: return S("Ashe");
		case 0x0206: return S("Echo");         case 0x0221: return S("Baptiste");
		case 0x0231: return S("Kiriko");       case 0x0236: return S("Junker Queen");
		case 0x023B: return S("Sigma");        case 0x028D: return S("Ramattra");
		case 0x0291: return S("LifeWeaver");   case 0x030A: return S("Mauga");
		case 0x031C: return S("Illari");       case 0x032B: return S("Venture");
		case 0x04E7: return S("Juno");
		default: return nullptr;
	}
}

static inline int _ult(uint64_t comp, uint64_t link)
{
	constexpr uint64_t	v9 = 96ULL;
	uint64_t			sc;
	uint64_t			v6;
	uint32_t			cnt;
	uint64_t			ptr;
	uint64_t			entry;
	uint64_t			skill;
	float				charge;
	uint32_t			i;
	int					uc;

	sc = DecryptComponent(comp, TYPE_SKILL);
	if (sc < 0x10000ULL || sc >= 0x800000000000ULL)
		sc = DecryptComponent(link, TYPE_SKILL);
	if (sc < 0x10000ULL || sc >= 0x800000000000ULL) return -1;

	uc = -1;
	__try {
		v6  = sc + 0x5B0;
		cnt = SDK->RPM<uint32_t>(v6 + v9 + 8);
		ptr = SDK->RPM<uint64_t>(v6 + v9);
		if (cnt > 0 && cnt < 512 && ptr > 0x10000ULL && ptr < 0x800000000000ULL) {
			entry = ptr + 16ULL * (cnt - 1);
			for (i = 0; i < cnt; i++, entry -= 16) {
				if (SDK->RPM<uint16_t>(entry) == 0x1e32) {
					skill = SDK->RPM<uint64_t>(entry + 8);
					if (skill > 0x10000ULL && skill < 0x800000000000ULL) {
						charge = SDK->RPM<float>(skill + 0x60);
						if (!isnan(charge) && charge >= 0.f) {
							if (charge <= 1.001f)
								uc = (int)(charge * 100.f + 0.5f);
							else if (charge <= 101.f)
								uc = (int)(charge + 0.5f);
						}
					}
					break;
				}
			}
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	return uc;
}

inline void DrawUltBars(ImDrawList* dl)
{
	float		sx;
	float		sy;
	int			uc;
	float		pct;
	float		bx;
	float		by;
	uint32_t	col;

	if (!g_ultBars) return;
	for (auto& e : g_entities) {
		if (e.isLocal) continue;
		if (g_teamCheck && g_localTeam && e.team == g_localTeam) continue;
		if (!e.hasBones || e.health <= 0.f) continue;
		if (!e.heroId || !_hname(e.heroId)) continue;
		if (e.hasBones) {
			if (!WorldToScreen(e.bones[0], sx, sy)) continue;
		} else {
			if (!WorldToScreen(e.pos, sx, sy)) continue;
		}
		uc  = _ult(e.common, e.addr);
		if (uc < 0) continue;
		pct = uc / 100.f;
		bx  = sx - 20.f;
		by  = sy - 28.f;
		col = uc >= 100 ? IM_COL32(255, 60, 60, 230) : IM_COL32(255, 200, 50, 200);
		dl->AddRectFilled(ImVec2(bx, by),        ImVec2(bx+40.f, by+4.f),      IM_COL32(0,0,0,130));
		dl->AddRectFilled(ImVec2(bx, by),        ImVec2(bx+40.f*pct, by+4.f),  col);
		dl->AddRect      (ImVec2(bx, by),        ImVec2(bx+40.f, by+4.f),      IM_COL32(80,80,80,160));
	}
}

inline void DrawUltPanel(ImDrawList* dl)
{
	float		px;
	float		py;
	float		bw;
	float		bh;
	float		bx;
	float		pct;
	int			uc;
	int			n;
	uint32_t	fillCol;
	char		buf[8];
	const char*	name;

	if (!g_ultPanel) return;
	px = 10.f;
	py = 50.f;
	bw = 90.f;
	bh = 10.f;
	n  = 0;

	for (auto& e : g_entities) {
		if (e.isLocal) continue;
		if (g_localTeam && e.team == g_localTeam) continue;
		if (!e.hasBones || e.health <= 0.f) continue;
		name = e.heroId ? _hname(e.heroId) : nullptr;
		if (!name) continue;

		uc   = _ult(e.common, e.addr);
		if (uc < 0) uc = 0;
		pct  = uc / 100.f;
		if (pct > 1.f) pct = 1.f;
		bx   = px + 80.f;

		dl->AddText(ImVec2(px, py), IM_COL32(220,220,220,230), name);
		dl->AddRectFilled(ImVec2(bx, py+1.f), ImVec2(bx+bw, py+bh+1.f),       IM_COL32(40,40,40,180), 3.f);
		fillCol = (pct >= 1.f) ? IM_COL32(0,255,255,255) : IM_COL32(255,80,80,220);
		dl->AddRectFilled(ImVec2(bx, py+1.f), ImVec2(bx+bw*pct, py+bh+1.f),   fillCol, 3.f);
		snprintf(buf, sizeof(buf), S("%d%%"), uc);
		dl->AddText(ImVec2(bx+bw+5.f, py), IM_COL32(255,255,255,200), buf);

		py += 22.f;
		if (++n >= 12) break;
	}
}
