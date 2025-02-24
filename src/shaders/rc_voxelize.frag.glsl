# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_EXT_nonuniform_qualifier: require

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


layout(binding = 0, set = 1) uniform sampler2D textures[];


layout(push_constant) uniform block
{
    VoxelizationConsts consts;
};

layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws [];
};


// read / write
layout(binding = 3) buffer FragCount
{
    uint fragCount;
};

// write
layout(binding = 4, R32UI) uniform writeonly uimageBuffer out_wpos;
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

    // clip space to voxel space, [0, 1] to [0, voxGridCount]
    vec3 pos = vec3(in_pos.xy * .5f + vec2(.5f), in_pos.z);

    ivec3 ipos = 
        ivec3(float(pos.x) * float(consts.voxGridCount) + .5f
            , float(pos.y) * float(consts.voxGridCount) + .5f
            , float(pos.z) * float(consts.voxGridCount) + .5f);

    uint voxIdx = ipos.z * consts.voxGridCount * consts.voxGridCount + ipos.y * consts.voxGridCount + ipos.x;

    uint uidx = atomicAdd(fragCount, 1u);
    voxels[voxIdx] = uidx;

    MeshDraw mDraw = meshDraws[in_drawId];
    vec4 albedo = vec4(0, 0, 0, 1);
    if (mDraw.albedoTex > 0)
    {
        albedo = texture(textures[nonuniformEXT(mDraw.albedoTex)], in_uv);
    }

    // temproray using albedo combines the emission info
    albedo.w = mDraw.emissiveTex > 0 ? 1 : 0;

    imageStore(out_wpos, int(uidx), ivec4(voxIdx, 0, 0, 0));
    imageStore(out_albedo, int(uidx), albedo);
    imageStore(out_norm, int(uidx), vec4(0.5));
}