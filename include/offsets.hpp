#pragma once
#include <cstdint>

namespace offset {
    constexpr uint64_t Address_entity_base  = 0x3935908;
    constexpr uint64_t OW_COMPONENT_QWORD   = 0x3A92E70;
    constexpr uint64_t OW_COMPONENT_BYTE    = 0x377E243;
    constexpr uint64_t OW_VIEWMATRIX_ENC    = 0x38DC230;
    constexpr uint64_t GlobalKeyPtr         = 0x3A92E70;
    constexpr uint64_t outline_offsets      = 0x97;
    constexpr uint64_t bytekey_outline      = 0x377E6E4;
    constexpr uint64_t ULT_OFF              = 0x2C0;
}

enum eComponentType : int32_t {
    TYPE_TEAM              = 0x21,
    TYPE_LINK              = 0x34,
    TYPE_P_VISIBILITY      = 0x35,
    TYPE_HEALTH            = 0x3B,
    TYPE_VELOCITY          = 0x4,
    TYPE_BONE              = 0x27,
    TYPE_ROTATION          = 0x2F,
    TYPE_PLAYERCONTROLLER  = 0x43,
    TYPE_OUTLINE           = 0x5B,
    TYPE_ABILITY           = 0x86,
    TYPE_SKILL             = 0x37,
};

static const int g_boneIds[18] = { 17, 16, 81, 82, 49, 54, 14, 51, 86, 96, 87, 97, 41, 71, 99, 89, 100, 90 };

static const int g_boneLines[17][2] = {
    {0,1}, {1,2}, {2,3},
    {1,4}, {4,6}, {6,12},
    {1,5}, {5,7}, {7,13},
    {3,8}, {8,10}, {10,15},
    {3,9}, {9,11}, {11,14},
    {15,17}, {14,16}
};