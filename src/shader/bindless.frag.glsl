# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require
# extension GL_EXT_nonuniform_qualifier: require

# include "debug_gpu.h"
# include "mesh_gpu.h"
# include "math.h"
# include "pbr.h"


layout(location = 0) in flat uint in_drawId;
layout(location = 1) in vec3 in_wPos;
layout(location = 2) in vec3 in_norm;
layout(location = 3) in vec4 in_tan;
layout(location = 4) in vec2 in_uv;
layout(location = 5) in flat uint in_triId;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_wPos;
layout(location = 3) out vec4 out_emissive;
layout(location = 4) out vec4 out_specular;

layout(binding = 0, set = 1) uniform sampler2D textures[];

layout(push_constant) uniform block
{
    Constants consts;
};

// readonly
layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws [];
};


void main()
{
#if DEBUG_MESHLET
	uint mhash = hash(in_drawId);
	out_emissive = vec4(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255), 255) / 255.0;
#else
    MeshDraw mDraw = meshDraws[in_drawId];
    
    vec3 wPos = (in_wPos / consts.probeRangeRadius) * 0.5f + .5f; // normalize to [0, 1]

    vec4 albedo = vec4(0.5, 0.5, 0.5, 1.0);
    if (mDraw.albedoTex > 0) { 
        albedo = texture(textures[nonuniformEXT(mDraw.albedoTex)], in_uv);
    }

    vec4 normal = vec4(0.0, 0.0, 1.0, 0.0);
    if (mDraw.normalTex > 0)
    {
        normal = texture(textures[nonuniformEXT(mDraw.normalTex)], in_uv) * 2.0 - 1.0;
    }

    vec3 bitan = cross(in_norm, in_tan.xyz) * in_tan.w;
    vec3 n = normalize(normal.x * in_tan.xyz + normal.y * bitan + normal.z * in_norm);

    vec4 specular = vec4(0.04, 0.04, 0.04, 1.0);
    if (mDraw.specularTex > 0)
    {
        specular = texture(textures[nonuniformEXT(mDraw.specularTex)], in_uv);
    }

    vec4 emissive = vec4(0.0, 0.0, 0.0, 1.0);
    if (mDraw.emissiveTex > 0)
    {
        emissive = texture(textures[nonuniformEXT(mDraw.emissiveTex)], in_uv);

        out_albedo = albedo;
        out_normal = vec4(n, 1.0);
        out_wPos = vec4(wPos, 1.0);
        out_emissive = vec4(emissive.rgb, 1.0);
        out_specular = specular;
        return;
    }

    out_albedo = vec4(albedo.xyz, 1.0);
    out_normal = vec4(n, 1.0);
    out_wPos = vec4(wPos, 1.0);
    out_emissive = vec4(emissive.rgb, 1.0);
    out_specular = vec4(specular.rgb, 1.0);
#endif
}
