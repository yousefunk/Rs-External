#include "memory.hpp"
#include <psapi.h>
#include <unordered_set>

void ReadEntities() {
    g_entities.clear();
    if (!SDK || !SDK->hProcess) return;
    struct Ent { uint64_t e; uint64_t pad; };
    uint64_t lp = SDK->RPM<uint64_t>(SDK->dwGameBase + offset::Address_entity_base);
    if (!lp) return;
    MEMORY_BASIC_INFORMATION mbi{};
    VirtualQueryEx(SDK->hProcess, (LPCVOID)lp, &mbi, sizeof(mbi));
    SIZE_T cnt = mbi.RegionSize / sizeof(Ent);
    if (cnt > 4096) cnt = 4096;
    Ent* raw = (Ent*)malloc(cnt * sizeof(Ent));
    if (!raw) return;
    ReadProcessMemory(SDK->hProcess, (LPCVOID)lp, raw, cnt * sizeof(Ent), nullptr);

    if (g_componentCache.size() > 4096) {
        g_componentCache.clear();
    }

    std::unordered_map<uint32_t, uint64_t> commonMap;
    commonMap.reserve(cnt);
    for (size_t x = 0; x < cnt; x++) {
        uint64_t e = raw[x].e;
        if (!e) continue;
        uint64_t bm0 = SDK->RPM<uint64_t>(e + 0x110);
        if (!(bm0 & (1ULL << 0x3B))) continue;
        uint32_t uid = SDK->RPM<uint32_t>(e + 0x138);
        commonMap[uid] = e;
    }

    std::unordered_set<uint64_t> seen;
    for (size_t i = 0; i < cnt; i++) {
        uint64_t a = raw[i].e;
        if (!a) continue;
        uint64_t lnk = GetDecryptedComponent(a, TYPE_LINK);
        if (!lnk) continue;
        uint32_t uid = SDK->RPM<uint32_t>(lnk + 0xD4);
        
        uint64_t common = 0;
        auto it = commonMap.find(uid);
        if (it != commonMap.end() && it->second != a) {
            common = it->second;
        }

        float hp = 0, hp_max = 0;
        float3 pos = {};
        float rot_y = 0;
        float3 bones[18] = {};
        int boneCount = 0;
        bool hasBones = false;
        if (common && !seen.count(common)) {
            seen.insert(common);
            uint64_t hc = GetDecryptedComponent(common, TYPE_HEALTH);
            if (hc) { hp = SDK->RPM<float>(hc + 0xE0); hp_max = SDK->RPM<float>(hc + 0xDC); }
            uint64_t vc = GetDecryptedComponent(common, TYPE_VELOCITY);
            if (vc) {
                pos = SDK->RPM<float3>(vc + 0x200);
                pos.y -= 1.0f;
                uint64_t rc = GetDecryptedComponent(common, TYPE_ROTATION);
                if (rc) {
                    uint64_t rp = SDK->RPM<uint64_t>(rc + 0x888);
                    if (rp) {
                        float3 rot = SDK->RPM<float3>(rp + 0x90C);
                        rot_y = rot.x;
                    }
                }
                uint64_t pBoneData = SDK->RPM<uint64_t>(vc + 0x8B0);
                if (pBoneData) {
                    uint64_t bonesBase = SDK->RPM<uint64_t>(pBoneData + 0x20);
                    if (bonesBase) {
                        int indices[18];
                        for (int bi = 0; bi < 18; bi++)
                            indices[bi] = get_bone_array_index(pBoneData, g_boneIds[bi]);
                        int uniqueCount = 0;
                        for (int bi = 0; bi < 18; bi++) {
                            bool dup = false;
                            for (int bj = 0; bj < bi; bj++)
                                if (indices[bi] == indices[bj]) { dup = true; break; }
                            if (!dup) uniqueCount++;
                        }
                        if (uniqueCount >= 12) {
                            boneCount = 18;
                            for (int bi = 0; bi < 18; bi++) {
                                float3 lb = SDK->RPM<float3>(bonesBase + (0x30 * indices[bi]) + 0x20);
                                float3 rot = rotate_y(lb, rot_y);
                                bones[bi] = { rot.x + pos.x, rot.y + pos.y, rot.z + pos.z };
                            }
                            hasBones = true;
                        } else {
                            static const int botBoneIds[5] = {17, 16, 3, 13, 54};
                            boneCount = 5;
                            for (int bi = 0; bi < 5; bi++) {
                                int idx = get_bone_array_index(pBoneData, botBoneIds[bi]);
                                float3 lb = SDK->RPM<float3>(bonesBase + (0x30 * idx) + 0x20);
                                float3 rot = rotate_y(lb, rot_y);
                                bones[bi] = { rot.x + pos.x, rot.y + pos.y, rot.z + pos.z };
                            }
                            hasBones = true;
                        }
                    }
                }
                bool isLocal = false;
                uint64_t pcTry = GetDecryptedComponent(a, TYPE_PLAYERCONTROLLER);
                if (!pcTry) pcTry = GetDecryptedComponent(common, TYPE_PLAYERCONTROLLER);
                bool hasPC = (pcTry != 0);
                if (hasPC) {
                    isLocal = true;
                } else if (boneCount > 0) {
                    float dx = bones[0].x - g_cameraPos.x;
                    float dy = bones[0].y - g_cameraPos.y;
                    float dz = bones[0].z - g_cameraPos.z;
                    if (sqrtf(dx*dx + dy*dy + dz*dz) < 3.0f) isLocal = true;
                }
                int entityTeam = 0;
                uint64_t tb = GetDecryptedComponent(common, TYPE_TEAM);
                if (tb) {
                    uint32_t teamVal = SDK->RPM<uint32_t>(tb + 0x58);
                    if (teamVal & (1 << 23)) entityTeam = 1;
                    else if (teamVal & (1 << 24)) entityTeam = 2;
                    else if (teamVal & (1 << 27)) entityTeam = 5;
                    else entityTeam = (int)(teamVal & 0xFF) + 10;
                }
                bool entityVis = true;
                uint64_t vb = GetDecryptedComponent(a, TYPE_P_VISIBILITY);
                if (vb) entityVis = DecryptVis(vb);
                uint16_t heroId = 0;
                uint64_t hc2 = GetDecryptedComponent(a, 0x54);
                if (hc2) { uint64_t hid64 = SDK->RPM<uint64_t>(hc2 + 0xD0); heroId = (uint16_t)(hid64 & 0xFFFF); }

                EntityInfo ei = {};
                ei.addr = a; ei.common = common; ei.heroId = heroId;
                ei.uid = uid; ei.health = hp; ei.health_max = hp_max; ei.pos = pos;
                ei.rot_y = rot_y; ei.isLocal = isLocal; ei.visible = entityVis;
                ei.hasBones = hasBones; ei.hasPC = hasPC; ei.boneCount = boneCount;
                ei.team = entityTeam; ei.pBoneData = pBoneData;
                if (boneCount > 0) memcpy(ei.bones, bones, sizeof(bones));
                g_entities.push_back(ei);
            }
        }
    }
    free(raw);
}

void ReadViewMatrix() {
    uint64_t enc = SDK->RPM<uint64_t>(SDK->dwGameBase + offset::OW_VIEWMATRIX_ENC);
    uint64_t addr = (enc + 0x37316FB2858F0E4ALL) ^ 0xB6326CCBCA7E34F4uLL;
    uint64_t ptr1 = SDK->RPM<uint64_t>(addr + 0x20);
    uint64_t ptr2 = SDK->RPM<uint64_t>(ptr1 + 0x48);
    uint64_t ptr3 = SDK->RPM<uint64_t>(ptr2 + 0x6C8);
    uint64_t ptr4 = SDK->RPM<uint64_t>(ptr3 + 0x8);
    g_viewMatrix = SDK->RPM<Matrix>(ptr4 + 0xC0);
    Matrix viewMtx = SDK->RPM<Matrix>(ptr2 + 0x140);
    float* d = viewMtx.data;
    float A = d[5]*d[10] - d[9]*d[6];
    float B = d[9]*d[2] - d[1]*d[10];
    float C = d[1]*d[6] - d[5]*d[2];
    float Z = d[0]*A + d[4]*B + d[8]*C;
    if (fabsf(Z) > 0.0001f) {
        float D = d[8]*d[6] - d[4]*d[10];
        float E = d[0]*d[10] - d[8]*d[2];
        float F = d[4]*d[2] - d[0]*d[6];
        float G = d[4]*d[9] - d[8]*d[5];
        float H = d[8]*d[1] - d[0]*d[9];
        float K = d[0]*d[5] - d[4]*d[1];
        g_cameraPos.x = -(d[12]*A + d[13]*D + d[14]*G) / Z;
        g_cameraPos.y = -(d[12]*B + d[13]*E + d[14]*H) / Z;
        g_cameraPos.z = -(d[12]*C + d[13]*F + d[14]*K) / Z;
    }
}

bool WorldToScreen(float3 world, float& sx, float& sy) {
    float* m = g_viewMatrix.data;
    float w = m[3] * world.x + m[7] * world.y + m[11] * world.z + m[15];
    if (w < 0.01f) return false;
    float x = m[0] * world.x + m[4] * world.y + m[8] * world.z + m[12];
    float y = m[1] * world.x + m[5] * world.y + m[9] * world.z + m[13];
    float invW = 1.0f / w;
    sx = (g_screenW * 0.5f) * (1.0f + x * invW);
    sy = (g_screenH * 0.5f) * (1.0f - y * invW);
    return true;
}