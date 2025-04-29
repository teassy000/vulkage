#pragma once

#include "kage_math.h"
#include "common.h"
#include "kage.h"
#include "demo_structs.h"

enum class RCDbgIndexType : uint32_t
{
    probe_first = 0,
    ray_first,
    count
};

enum class RCDbgColorType : uint32_t
{
    albedo = 0,
    normal,
    worldPos,
    emissive,
    brxDistance,
    brxNormal,
    rayDir,
    probePos,
    probeHash,
    count
};

enum class rc2dStage : uint32_t
{
    build = 0,
    merge,
    use,
    count
};