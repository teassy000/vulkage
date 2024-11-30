# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require
# extension GL_EXT_nonuniform_qualifier: require

# include "mesh_gpu.h"


layout(location = 0) in flat uint out_drawId;
layout(location = 1) in vec3 in_wPos;
layout(location = 2) in vec3 in_norm;
layout(location = 4) in vec4 in_tang;
layout(location = 3) in vec2 in_uv;


layout(location = 0) out vec4 outputColor;

layout(binding = 0, set = 1) uniform sampler2D textures[];

// readonly
layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws [];
};

uint hash(uint a)
{
    a = (a + 0x7ed55d16) + (a << 12);
    a = (a ^ 0xc761c23c) ^ (a >> 19);
    a = (a + 0x165667b1) + (a << 5);
    a = (a + 0xd3a2646c) ^ (a << 9);
    a = (a + 0xfd7046c5) + (a << 3);
    a = (a ^ 0xb55a4f09) ^ (a >> 16);
    return a;
}
void main()
{
#if DEBUG_MESHLET
	uint mhash = hash(out_drawId);
	outputColor = vec4(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255), 255) / 255.0;
#else
    MeshDraw mDraw = meshDraws[out_drawId];

    vec4 albedo = vec4(0.5, 0.5, 0.5, 1.0);
    if (mDraw.albedoTex > 0) { 
        albedo = texture(textures[nonuniformEXT(mDraw.albedoTex)], in_uv);
    }

    outputColor = albedo;
#endif
}
