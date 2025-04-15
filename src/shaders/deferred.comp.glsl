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

layout(binding = 0) readonly buffer RadianceConstants
{
    RCAccessData rcAccesses [];
};

layout(binding = 1) uniform sampler2D in_albedo;
layout(binding = 2) uniform sampler2D in_normal;
layout(binding = 3) uniform sampler2D in_wPos;
layout(binding = 4) uniform sampler2D in_emmision;
layout(binding = 5) uniform sampler2D in_specular;
layout(binding = 6) uniform sampler2D in_sky;

layout(binding = 7) uniform sampler2DArray in_radianceCascade;
layout(binding = 8) uniform writeonly image2D out_color;

layout(binding = 0, set = 2) uniform sampler2DArray in_rcMerged[MAX_RADIANCE_CASCADES];

ivec3 getNearestProbeIdx(vec3 _wPos, float _radius, float _probeSideLen)
{
    vec3 mappedPos = _wPos + vec3(_radius); // map to [0, 2 * radius]
    return ivec3(floor(mappedPos / _probeSideLen));
}

vec3 brdfSpecular(float _nov, float _nol, float _noh, float _loh, vec3 _f0, float _linearRough)
{
    // Schlick Fresnel
    vec3 F = F_Schlick(_f0, _loh);
    // GGX Normal Distribution
    float D = D_GGX(_linearRough, _noh);
    // Smith Visibility
    float V = V_SmithGGXCorrelated(_linearRough, _nov, _nol);
    
    return F * D * V;
}

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;
    vec2 uv = vec2((pos.x + .5f) / consts.w, (pos.y + .5f) / consts.h);
    vec3 camPos = vec3(consts.camx, consts.camy, consts.camz);

    vec4 albedo = texture(in_albedo, uv);
    vec4 normal = texture(in_normal, uv);
    vec4 emmision = texture(in_emmision, uv);
    vec4 sky = texture(in_sky, uv);
    vec3 wPos = texture(in_wPos, uv).xyz;
    vec4 specular = texture(in_specular, uv);
    float radius = consts.totalRadius;
    wPos = (wPos * 2.f - 1.f) * radius; // [0.f, 1.f] to [-radius, +radius]

#if DEBUG_MESHLET
    emmision.xyz *= 0.7f;
    imageStore(out_color, ivec2(pos), emmision);
#else

    vec4 rayCol = vec4(0.f);
    vec3 rayDir = vec3(0.f);

    // locate the cascade based on the wpos
    for (uint ii = consts.startCascade; ii <= consts.endCascade; ++ii)
    {
        const RCAccessData rcAccess = rcAccesses[ii];
        uint prob_sideCount = rcAccess.probeSideCount;
        uint ray_sideCount = rcAccess.raySideCount;
        float prob_sideLen = rcAccess.probeSideLen;
        uint layerOffset = rcAccess.layerOffset;

        vec3 mappedPos = wPos - camPos + vec3(radius); // map to camera related pos then shift to [0, 2 * radius]
        ivec3 probeIdx = ivec3(floor(mappedPos / prob_sideLen));
        vec3 probeCenter = getCenterWorldPos(probeIdx, radius, prob_sideLen) + camPos; // probes follows the camera and in world space
        vec3 rd = normalize(wPos.xyz - probeCenter);
        
        vec2 rayUV = float32x3_to_oct(rd);
        rayUV = rayUV * .5f + .5f; // map octrahedral uv to [0, 1]

        ivec2 rayIdx = ivec2(rayUV * float(ray_sideCount));
        ivec2 pixelIdx = ivec2(0u);
        // now base on the index type, locate the ray in the cascade
        switch (consts.debugIdxType)
        {
            case 0:  // probe first index
                pixelIdx = probeIdx.xy * int(ray_sideCount) + rayIdx;
                break;
            case 1: // ray first index
                pixelIdx = rayIdx * int(prob_sideCount) + probeIdx.xy;
                break;
        }

        vec2 cascadeUV = (vec2(pixelIdx) + vec2(0.5f)) / float(prob_sideCount * ray_sideCount);
        uint layerIdx = probeIdx.z + layerOffset;

        vec4 rcc = texture(in_radianceCascade, vec3(cascadeUV, layerIdx));
        if (!compare(rcc, vec4(0.f)))
        {
            rayCol = rcc;// rcc * (1.f / float(ii));
            rayDir = rd;
            break;
        }
    }

    
    float lightIntensity = 1.0;
    float indirectIntensity = 0.32;

    float occlusion = specular.r;
    float roughness = specular.g;
    float matalness = specular.b;
    //vec3 baseColor = vec3(specular.r); // for env-test
    vec3 baseColor = albedo.rgb; // for bistor

    vec3 lightColor = rayCol.xyz;//vec3(0.98, 0.92, 0.89);

    vec3 l = rayDir;//normalize(vec3(0.7, 1.0, 0.7)); // in world space, from surface to light source
    vec3 v = normalize(vec3(camPos - wPos)); // from surface to observer

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
    vec3 Fr = brdfSpecular(NoV, NoL, NoH, LoH, f0, linearRoughness);

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

    //color = color * (1.0 - occlusion) + rayCol.rgb * occlusion;
    //color = rayCol.rgb;
    //color = Fr;

    if (albedo.a < 0.5 || normal.w < 0.02)
        color = sky.xyz;

    imageStore(out_color, ivec2(pos), vec4(color, 1.f));
#endif
}