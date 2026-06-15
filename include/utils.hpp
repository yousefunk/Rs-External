#pragma once
#include "sdk.hpp"
#include "offsets.hpp"
#include <intrin.h>
#include <cmath>

inline float3 rotate_y(float3 v, float angle) {
    float c = cosf(angle), s = sinf(angle);
    return { v.x * c + v.z * s, v.y, -v.x * s + v.z * c };
}

inline int get_bone_array_index(uint64_t pBoneData, int boneId) {
    int result = 0;
    __try {
        uint64_t inner = SDK->RPM<uint64_t>(pBoneData);
        uint32_t* tbl = (uint32_t*)SDK->RPM<uint64_t>(inner + 0x38);
        uint16_t cnt = SDK->RPM<uint16_t>(inner + 0x64);
        for (int i = 0; i < cnt; i++) {
            if (SDK->RPM<uint16_t>((uint64_t)(tbl + i)) == boneId) {
                result = i; break;
            }
        }
    } __except (1) {}
    return result;
}

inline float3 get_bone_world_pos(uint64_t veloComp, int boneId, float3 epos, float rotY) {
    __try {
        uint64_t pBoneData = SDK->RPM<uint64_t>(veloComp + 0x8B0);
        if (!pBoneData) return {};
        uint64_t bonesBase = SDK->RPM<uint64_t>(pBoneData + 0x20);
        if (!bonesBase) return {};
        int idx = get_bone_array_index(pBoneData, boneId);
        float3 bone = SDK->RPM<float3>(bonesBase + (0x30 * idx) + 0x20);
        float3 rot = rotate_y(bone, rotY);
        return { rot.x + epos.x, rot.y + epos.y, rot.z + epos.z };
    } __except (1) { return {}; }
}

inline bool get_3dbox_corners(uint64_t pBoneData, float3 epos, float rotY, float3 corners[8]) {
    __try {
        if (!pBoneData) return false;
        uint64_t bonesBase = SDK->RPM<uint64_t>(pBoneData + 0x20);
        if (!bonesBase) return false;
        int idx = get_bone_array_index(pBoneData, 17);
        float3 localHead = SDK->RPM<float3>(bonesBase + (0x30 * idx) + 0x20);
        localHead.y += 0.3f;
        float3 top[4] = {
            {localHead.x - 0.5f, localHead.y, localHead.z - 0.5f},
            {localHead.x - 0.5f, localHead.y, localHead.z + 0.5f},
            {localHead.x + 0.5f, localHead.y, localHead.z - 0.5f},
            {localHead.x + 0.5f, localHead.y, localHead.z + 0.5f},
        };
        localHead.y -= 1.5f;
        float3 bot[4] = {
            {localHead.x - 0.5f, localHead.y, localHead.z - 0.5f},
            {localHead.x - 0.5f, localHead.y, localHead.z + 0.5f},
            {localHead.x + 0.5f, localHead.y, localHead.z - 0.5f},
            {localHead.x + 0.5f, localHead.y, localHead.z + 0.5f},
        };
        for (int i = 0; i < 4; i++) {
            corners[i]     = { rotate_y(top[i], rotY).x + epos.x, rotate_y(top[i], rotY).y + epos.y, rotate_y(top[i], rotY).z + epos.z };
            corners[i + 4] = { rotate_y(bot[i], rotY).x + epos.x, rotate_y(bot[i], rotY).y + epos.y, rotate_y(bot[i], rotY).z + epos.z };
        }
        return true;
    } __except (1) { return false; }
}

inline bool DecryptVis(uint64_t InVisibilityComponent) {
    if (!InVisibilityComponent) return true;
    __try {
        uint64_t encrypted = SDK->RPM<uint64_t>(InVisibilityComponent + 0x98);
        uint64_t var_qword = SDK->RPM<uint64_t>(SDK->dwGameBase + 0x3A92E70);
        uint8_t var_byte = SDK->RPM<uint8_t>(SDK->dwGameBase + 0x377E893);
        uint64_t decrypted = _rotr64(encrypted - 0x20E7AF6FB94361E2ULL, 0x37)
            ^ SDK->RPM<uint64_t>(var_qword + 0xB2)
            ^ var_byte
            ^ 0xAD17A4349D1E3445ULL;
        decrypted = _rotr64(decrypted ^ 0x59FF47477EB2FE6ELL, 0x3E);
        uint64_t rotated = (decrypted >> 0x16) | (decrypted << 0x2A);
        return rotated == 1;
    } __except (1) { return true; }
}

inline uint64_t DecryptComponent(uint64_t entity, uint8_t component_id) {
    uint64_t bit = component_id & 0x3F;
    uint64_t high_bit_mask = (1ULL << bit);
    uint64_t low_bits_mask = high_bit_mask - 1ULL;
    uint64_t index = component_id >> 6;
    uint64_t component_bitmap = SDK->RPM<uint64_t>(entity + (8 * index) + 0x110);
    if (!(component_bitmap & high_bit_mask)) return 0;
    uint64_t isolated_bitmap = low_bits_mask & component_bitmap;
    uint64_t tmp1 = isolated_bitmap - ((isolated_bitmap >> 1) & 0x5555555555555555ULL);
    uint64_t tmp2 = (tmp1 & 0x3333333333333333ULL) + ((tmp1 >> 2) & 0x3333333333333333ULL);
    uint64_t tmp3 = (tmp2 + (tmp2 >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    uint64_t bit_count = (0x101010101010101ULL * tmp3) >> 56;
    uint64_t component_table   = SDK->RPM<uint64_t>(entity + 0x80);
    uint8_t  table_entry_index = SDK->RPM<uint8_t>(entity + index + 0x130);
    uint64_t qword_ptr         = SDK->RPM<uint64_t>(SDK->dwGameBase + offset::OW_COMPONENT_QWORD);
    uint8_t  enc_byte          = SDK->RPM<uint8_t>(SDK->dwGameBase + offset::OW_COMPONENT_BYTE);
    uint64_t component_salt    = SDK->RPM<uint64_t>(qword_ptr + 0x1D4);
    uint64_t slot_index = table_entry_index + bit_count;
    uint64_t component_ptr = SDK->RPM<uint64_t>(component_table + (slot_index * 8));
    uint64_t inner = (component_salt ^ component_ptr) ^ 0xDC01B58B9BDFFB4B;
    inner = _rotr64(inner, 0x20) + 0x24620C984E36588ULL;
    inner = (inner ^ enc_byte) - 0x7D957CD64821F39B;
    inner = _rotr64(inner, 0x3C);
    inner = _rotr64(inner, 0x39);
    return inner;
}

inline uint64_t GetDecryptedComponent(uint64_t entity, uint8_t component_id) {
    if (!entity) return 0;
    auto& cache = g_componentCache[entity];
    switch (component_id) {
        case TYPE_LINK:
            if (!cache.link_cached) { cache.link = DecryptComponent(entity, TYPE_LINK); cache.link_cached = true; }
            return cache.link;
        case TYPE_HEALTH:
            if (!cache.health_cached) { cache.health = DecryptComponent(entity, TYPE_HEALTH); cache.health_cached = true; }
            return cache.health;
        case TYPE_VELOCITY:
            if (!cache.velocity_cached) { cache.velocity = DecryptComponent(entity, TYPE_VELOCITY); cache.velocity_cached = true; }
            return cache.velocity;
        case TYPE_ROTATION:
            if (!cache.rotation_cached) { cache.rotation = DecryptComponent(entity, TYPE_ROTATION); cache.rotation_cached = true; }
            return cache.rotation;
        case TYPE_PLAYERCONTROLLER:
            if (!cache.pc_cached) { cache.pc = DecryptComponent(entity, TYPE_PLAYERCONTROLLER); cache.pc_cached = true; }
            return cache.pc;
        case TYPE_TEAM:
            if (!cache.team_cached) { cache.team = DecryptComponent(entity, TYPE_TEAM); cache.team_cached = true; }
            return cache.team;
        case TYPE_P_VISIBILITY:
            if (!cache.visibility_cached) { cache.visibility = DecryptComponent(entity, TYPE_P_VISIBILITY); cache.visibility_cached = true; }
            return cache.visibility;
        case 0x54:
            if (!cache.hc2_cached) { cache.hc2 = DecryptComponent(entity, 0x54); cache.hc2_cached = true; }
            return cache.hc2;
        default:
            return DecryptComponent(entity, component_id);
    }
}

inline const char* vk_to_str(int key) {
    if (key == VK_RBUTTON) return S("RMB");
    if (key == VK_LBUTTON) return S("LMB");
    if (key == VK_MBUTTON) return S("MMB");
    if (key >= 'A' && key <= 'Z') { static char s[2]; s[0] = (char)key; s[1] = 0; return s; }
    if (key >= '0' && key <= '9') { static char s[2]; s[0] = (char)key; s[1] = 0; return s; }
    return S("Key");
}