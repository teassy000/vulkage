#pragma once

struct BrixelBufDescs
{
    kage::BufferHandle buf;
    uint32_t size;
    uint32_t stride;
};

// Based on FfxBrixelizerTraceDebugModes
enum class BrixelDebugType : uint32_t
{
    distance = 0,       // distance to hit
    uvw,                // uvw at hit
    iterations,         // heatmap
    grad,               // normals at hit
    brick_id,
    cascade_id,
    count
};

struct BrixelDebugDescs
{
    float proj[16];
    float view[16];
    float camPos[3];

    uint32_t start_cas;
    uint32_t end_cas;
    float sdf_eps;
    float tmin;
    float tmax;
    BrixelDebugType debugType;
};


