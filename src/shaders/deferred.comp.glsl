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

#if ENABLE_RADIANCE_CASCADES


layout(binding = 6) readonly buffer RadianceConstants
{
    RCAccessData rcAccesses [];
};

layout(binding = 7) uniform sampler2DArray in_rcMergedProbe;
layout(binding = 8) uniform sampler2DArray in_rcMergedInverval;
layout(binding = 9) uniform writeonly image2D out_color;

#else

layout(binding = 6) uniform writeonly image2D out_color;

#endif // ENABLE_RADIANCE_CASCADES


struct Light
{
    vec3 dir;
    vec3 col;
};


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


ProbeSample getNearestProbeSample(vec3 _mappedPos, float _probeSideLen)
{
    vec3 fIdx = _mappedPos / _probeSideLen;
    
    ProbeSample samp;
    samp.baseIdx = ivec3(floor(fIdx));
    samp.ratio = fract(fIdx);

    return samp;
}

#if ENABLE_RADIANCE_CASCADES

vec4 getProbeColor(uint _probeSideCnt, ivec3 _probeIdx)
{
    vec2 uv = (vec2(_probeIdx) + 0.5f) / float(_probeSideCnt);

    vec4 rcc = texture(in_rcMergedProbe, vec3(uv, _probeIdx.z));

    return rcc;
}


vec4 trilinearColor(vec3 _mappedPos, float _probeSideLen, uint _probeSideCnt)
{

    ProbeSample samp = getNearestProbeSample(_mappedPos, _probeSideLen);

    float weights[8];
    trilinearWeights(samp.ratio, weights);

    vec4 outColor = vec4(0.f);
    for (uint ii = 0; ii < 8; ++ii)
    {
        ivec3 offset = getTrilinearProbeOffset(ii);
        ivec3 probeIdx = samp.baseIdx + offset;
        vec4 color = getProbeColor(_probeSideCnt, probeIdx);

        outColor += color * weights[ii];
    }

    return outColor;
}

#endif


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
    
    vec3 color = emmision.xyz;
    if (emmision.a < 0.5f)
        color = sky.xyz;

    imageStore(out_color, ivec2(pos), vec4(color, 1.f));
#else

    vec3 lightCol = vec3(0.98, 0.92, 0.89);
    vec3 lightDir = normalize(vec3(0.7, 1.0, 0.7)); // in world space, from surface to light source


#if ENABLE_RADIANCE_CASCADES
    // locate the cascade based on the wpos
    const uint currLv = consts.startCascade;
    const RCAccessData rcAccess = rcAccesses[currLv];
    uint prob_sideCount = rcAccess.probeSideCount;
    uint ray_sideCount = rcAccess.raySideCount;
    float prob_sideLen = rcAccess.probeSideLen;
    uint layerOffset = rcAccess.layerOffset;
#endif

    vec3 mappedPos = wPos - camPos + vec3(radius); // map to camera related pos then shift to [0, 2 * radius]

    float lightIntensity = 1.0;
    float indirectIntensity = 0.32;

    float occlusion = specular.r;
    float roughness = specular.g;
    float matalness = specular.b;
    //vec3 baseColor = vec3(specular.r); // for env-test
    vec3 baseColor = albedo.rgb; // for bistor

    vec3 v = normalize(vec3(camPos - wPos)); // from surface to observer

    // BRDF
    vec3 n = normal.xyz;
    vec3 h = normalize(v + lightDir);
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = saturate(dot(n, lightDir));
    float NoH = saturate(dot(n, h));
    float LoH = saturate(dot(lightDir, h));

    vec3 f0 = 0.04 * (1.0 - matalness) + baseColor.rgb * matalness;
    vec3 diffuseColor = (1.0 - matalness) * baseColor.rgb;

    float linearRoughness = roughness * roughness;

    // specular BRDF
    vec3 Fr = brdfSpecular(NoV, NoL, NoH, LoH, f0, linearRoughness);

    // diffuse BRDF
    vec3 Fd = diffuseColor * Fd_Burley(linearRoughness, NoV, NoL, LoH);
    
    vec3 color = Fd + Fr;
    color *= lightIntensity * lightCol * NoL;

    // diffuse indirect
    vec3 indirectDiffuse = Irradiance_SphericalHarmonics(n) * Fd_Lambert();
    vec3 ibl = indirectDiffuse * diffuseColor;
    color += ibl * indirectIntensity;

    color = Tonemap_ACES(color);
    color = OECF_sRGBFast(color);
    //color = rc_pc.rgb;

    if (albedo.a < 0.5 || normal.w < 0.02)
        color = sky.xyz;

    imageStore(out_color, ivec2(pos), vec4(color, 1.f));
#endif
}