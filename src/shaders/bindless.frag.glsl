# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require
# extension GL_EXT_nonuniform_qualifier: require

# include "mesh_gpu.h"
# include "math.h"
# include "pbr.h"


layout(location = 0) in flat uint out_drawId;
layout(location = 1) in vec3 in_wPos;
layout(location = 2) in vec3 in_norm;
layout(location = 3) in vec4 in_tan;
layout(location = 4) in vec2 in_uv;


layout(location = 0) out vec4 outputColor;

layout(binding = 0, set = 1) uniform sampler2D textures[];

// readonly
layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws [];
};

layout(binding = 6) readonly uniform Transform 
{
    TransformData trans;
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

    vec4 normal = vec4(0.0, 0.0, 1.0, 0.0);
    if (mDraw.normalTex > 0)
    {
        normal = texture(textures[nonuniformEXT(mDraw.normalTex)], in_uv) * 2.0 - 1.0;
    }

    vec4 specular = vec4(0.04, 0.04, 0.04, 1.0);
    if (mDraw.specularTex > 0)
    {
        specular = texture(textures[nonuniformEXT(mDraw.specularTex)], in_uv);
    }

    vec4 emissive = vec4(0.0, 0.0, 0.0, 1.0);
    if (mDraw.emissiveTex > 0)
    {
        emissive = texture(textures[nonuniformEXT(mDraw.emissiveTex)], in_uv);
        outputColor = vec4(emissive.rgb, 1.0);
        return;
    }

    vec3 bitan = cross(in_norm, in_tan.xyz) * in_tan.w;
    vec3 n = normalize(normal.x * in_tan.xyz + normal.y * bitan + normal.z * in_norm);

    float lightIntensity = 2.0;
    float indirectIntensity = 0.64;

    float occlusion = specular.r;
    float roughness = specular.g;
    float matalness = specular.b;
    //vec3 baseColor = vec3(specular.r); // for env-test
    vec3 baseColor = albedo.rgb; // for bistor

    vec3 lightColor = vec3(0.98, 0.92, 0.89);

    vec3 l = normalize(vec3(0.7, 1.0, 0.7)); // in world space, from surface to light source
    vec3 v = normalize(vec3(trans.cameraPos - in_wPos)); // from surface to observer

    // BRDF
    vec3 h = normalize(v + l);
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = saturate(dot(n, l));
    float NoH = saturate(dot(n, h));
    float LoH = saturate(dot(l, h));

    vec3 f0 = 0.04 * (1.0 - matalness) + baseColor.rgb * matalness;
    vec3 diffuseColor = (1.0 - matalness) * baseColor.rgb;

    float linearRoughness = roughness * roughness;
    
    // specular BRDF
    vec3 F = F_Schlick(f0, LoH);
    float D = D_GGX(linearRoughness, NoH);
    float V = V_SmithGGXCorrelated(linearRoughness, NoV, NoL);
    vec3 Fr = F * D * V;

    // diffuse BRDF
    vec3 Fd = diffuseColor * Fd_Burley(linearRoughness, NoV, NoL, LoH);
    vec3 color = Fd + Fr;
    color *= lightIntensity * lightColor * NoL;
    //color *= occlusion;

    // diffuse indirect
    vec3 indirectDiffuse = Irradiance_SphericalHarmonics(n) * Fd_Lambert();
    vec3 ibl = indirectDiffuse * diffuseColor;
    color += ibl * indirectIntensity;

    // specular indirect
    // ?
    // image based lighting?

    // diffuse occlusion
    // sample baked oc

    // specular occlusion
    // sample baked oc

    color = Tonemap_ACES(color);
    color = OECF_sRGBFast(color);
    if (albedo.a < 0.5)
         discard;

    vec3 innorm = (n + 1.0) * 0.5;
    outputColor = vec4(vec3(color), 1.0);
#endif
}
