#pragma once
#include "sdk.hpp"
#include "utils.hpp"
#include "offsets.hpp"

void ReadEntities();
void ReadViewMatrix();
bool WorldToScreen(float3 world, float& sx, float& sy);