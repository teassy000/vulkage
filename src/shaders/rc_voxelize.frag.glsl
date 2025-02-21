# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require

# include "debug_gpu.h"
# include "mesh_gpu.h"
# include "rc_common.h"

layout(location = 0) in flat uint in_drawId;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 in_tangent;
layout(location = 4) in vec3 in_pos;

layout(location = 5) in flat vec3 in_minAABB;
layout(location = 6) in flat vec3 in_maxAABB;

layout(push_constant) uniform block
{
    VoxelizationConfig config;
};

// read / write
layout(binding = 3) buffer FragCount
{
    uint fragCount;
};

// write
layout(binding = 4, RGBA16F) uniform writeonly imageBuffer out_wpos;
layout(binding = 5, RGBA8) uniform writeonly imageBuffer out_albedo;
layout(binding = 6, RGBA16F) uniform writeonly imageBuffer out_norm;

layout(binding = 7) buffer VoxelMap
{
    uint voxels[] ;
};

layout(location = 0) out vec4 out_dummy;

void main()
{
    
    if (any(lessThan(in_pos, in_minAABB)) || any(lessThan(in_maxAABB, in_pos)))
        discard;
    
    // clip space to voxel space, [0, 1] to [0, edgeLen]
    vec3 pos = vec3(in_pos.xy * .5f + vec2(.5f), in_pos.z);

    // z * edgeLen^2 + y * edgeLen + x
    float voxelVar = float(pos.z * float(config.edgeLen * config.edgeLen)) + float(pos.y * float(config.edgeLen)) + float(pos.x);
    uint voxelIdx = uint(voxelVar);


    uint uidx = atomicAdd(fragCount, 1u);
    voxels[uidx] = voxelIdx;

    vec3 wpos = vec3(pos - vec3(0.5f)) * 2.f;

    imageStore(out_wpos, int(uidx), vec4(wpos, 1));
    imageStore(out_albedo, int(uidx), vec4(0.5));
    imageStore(out_norm, int(uidx), vec4(0.5));
}