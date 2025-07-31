#version 450

#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_GOOGLE_include_directive : require

#include "mesh_gpu.h"
#include "math.h"

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(push_constant) uniform blocks{
	TerrainConstants tc;
};

layout(binding = 0) uniform sampler2D inSampler;

layout(binding = 1) writeonly buffer TerrainVertices
{
    TerrainVertex vertices[];
};

void main()
{
    uvec2 loc = gl_GlobalInvocationID.xy;

    float hTex = texelFetch(inSampler, ivec2(loc), 0).r;

    float height = hTex * tc.sv;

    vec3 vpos = vec3(loc.x * tc.sh, height, loc.y * tc.sh);

    uint idx = loc.x * tc.tch + loc.y;
    vertices[idx].vx = vpos.x + tc.bx;
    vertices[idx].vy = vpos.y + tc.by;
    vertices[idx].vz = vpos.z + tc.bz;
    vertices[idx].tu = (vpos.x + 0.5) / (tc.w * tc.sh);
    vertices[idx].tv = (vpos.z + 0.5) / (tc.h * tc.sh);
}