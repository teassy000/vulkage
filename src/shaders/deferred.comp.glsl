# version 450


# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require

#include "pbr.h"
#include "rc_common.h"
#include "debug_gpu.h"

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(push_constant) uniform blocks
{
    DeferredConstants consts;
};

layout(binding = 0) uniform sampler2D in_albedo;
layout(binding = 1) uniform sampler2D in_normal;
layout(binding = 2) uniform sampler2D in_wPos;
layout(binding = 3) uniform sampler2D in_emmision;
layout(binding = 4) uniform sampler2D in_specular;
layout(binding = 5) uniform sampler2D in_sky;

layout(binding = 6) uniform sampler2DArray in_radianceCascade;

layout(binding = 7) uniform writeonly image2D out_color;

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;
    vec2 uv = (vec2(pos) + vec2(.5)) / consts.imageSize;

    vec4 albedo = texture(in_albedo, uv);
    vec4 normal = texture(in_normal, uv);
    vec4 emmision = texture(in_emmision, uv);
    vec4 sky = texture(in_sky, uv);
    vec3 wPos = texture(in_wPos, uv).xyz;
    vec4 specular = texture(in_specular, uv);
    wPos = (wPos * 2.f - 1.f) * consts.sceneRadius;

#if DEBUG_MESHLET
    emmision.xyz *= 0.7f;
    imageStore(out_color, ivec2(pos), emmision);
#else

    /*
    vec4 color = albedo;

    // locate the cascade based on the wpos
    uint layerOffset = 0;
    for(uint ii = 0; ii < consts.cascade_lv; ++ii)
    {
        uint level_factor = uint(1u << ii);
        uint prob_sideCount = consts.cascade_0_probGridCount / level_factor;
        uint prob_count = uint(pow(prob_sideCount, 3));
        uint ray_sideCount = consts.cascade_0_rayGridCount * level_factor;
        uint ray_count = ray_sideCount * ray_sideCount; // each probe has a 2d grid of rays
        float prob_sideLen = consts.sceneRadius * 2.f / float(prob_sideCount);

        ivec3 probeIdx = ivec3(floor(wPos.xyz / prob_sideLen));
        uint layerIdx = probeIdx.z + layerOffset;

        // calc the probe center based on the probe index
        vec3 probeCenter = vec3(probeIdx) * prob_sideLen + prob_sideLen * 0.5f - vec3(consts.sceneRadius);

        // the direction of the ray, the opposite of the probe center to wPos
        vec3 rayDir = normalize(probeCenter - wPos.xyz);

        // map the uv to the ray grid
        vec2 raySubUV = octEncode(rayDir);
        raySubUV *= float(ray_sideCount); // to probe pix Idx
        raySubUV += 0.5f; // to center of the pix
        raySubUV += vec2(probeIdx.xy * prob_sideCount);
        raySubUV /= (ray_sideCount * prob_sideCount);

        vec4 col = texture(in_radianceCascade, ivec3(raySubUV.xy, layerIdx));
        if(col != vec4(0.f))
        {
            color *= 0.5f;
            color += col;
            break;
        }
    }
    */

    float lightIntensity = 2.0;
    float indirectIntensity = 0.32;

    float occlusion = specular.r;
    float roughness = specular.g;
    float matalness = specular.b;
    //vec3 baseColor = vec3(specular.r); // for env-test
    vec3 baseColor = albedo.rgb; // for bistor

    vec3 lightColor = vec3(0.98, 0.92, 0.89);

    vec3 l = normalize(vec3(0.7, 1.0, 0.7)); // in world space, from surface to light source
    vec3 v = normalize(vec3(consts.camPos - wPos)); // from surface to observer

    // BRDF
    vec3 n = normal.xyz;
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
    if (albedo.a < 0.5 || normal.w < 0.02)
        color = sky.xyz;

    imageStore(out_color, ivec2(pos), vec4(color, 1.f));
#endif
}