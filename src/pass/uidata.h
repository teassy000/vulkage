#pragma once
#include "core/kage.h"

struct DebugLogicData
{
    float posX, posY, posZ;
    float frontX, frontY, frontZ;
};

struct UIInput
{
    float mousePosx, mousePosy;

    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } mouseButtons;

    float width, height;
};
