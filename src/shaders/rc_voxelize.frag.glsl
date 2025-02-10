# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require

# include "debug_gpu.h"
# include "mesh_gpu.h"
# include "rc_common.h"


layout(push_constant) uniform block
{
    VoxelizationConfig config;
};

layout(binding = 3) readonly uniform Transform
{
    TransformData trans;
};

layout(binding = 4, RGBA8) uniform writeonly image3D out_albedo;

layout(location = 0) in flat uint in_drawId;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 in_tangent;
layout(location = 4) in vec3 in_pos;

layout(location = 5) in flat vec3 in_minAABB;
layout(location = 6) in flat vec3 in_maxAABB;


layout(location = 0) out vec4 out_dummy;

void main()
{
    
    if (any(lessThan(in_pos, in_minAABB)) || any(lessThan(in_maxAABB, in_pos)))
        discard;
    
    uint mhash = hash(in_drawId);
    vec3 color = vec3(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255)) / 255.0;

    // clip space to voxel space
    vec3 uv = in_pos;
    uv.xy = uv.xy * .5f + vec2(.5f);
    uv.y = 1.0 - uv.y;
    ivec3 uv_i = ivec3(uv * 256.f);

    imageStore(out_albedo, uv_i, vec4(color, 1.0));
}