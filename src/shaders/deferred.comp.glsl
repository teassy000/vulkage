# version 450


# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require

#include "pbr.h"
#include "rc_common.h"

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(push_constant) uniform blocks
{
    DeferredConstants consts;
};

layout(binding = 0) uniform sampler2D in_albedo;
layout(binding = 1) uniform sampler2D in_normal;
layout(binding = 2) uniform sampler2D in_wPos;
layout(binding = 3) uniform sampler2D in_emmision;
layout(binding = 4) uniform sampler2D in_sky;

layout(binding = 5) uniform sampler2DArray in_radianceCascade;

layout(binding = 6) uniform writeonly image2D out_color;

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;
    vec2 uv = (vec2(pos) + vec2(.5)) / consts.imageSize;

    vec4 albedo = texture(in_albedo, uv);
    vec4 normal = texture(in_normal, uv);
    vec3 wPos = texture(in_wPos, uv).xyz;
    wPos = (wPos * 2.f - 1.f) * consts.sceneRadius;

    vec4 emmision = texture(in_emmision, uv);
    vec4 sky = texture(in_sky, uv);

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
        vec3 probeCenter = vec3(probeIdx) * prob_sideLen + prob_sideLen * 0.5f - vec3(consts.sceneRadius) - consts.camPos;

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
            color = col;
            break;
        }
    }

    if (normal.w < 0.02)
        color = sky;

    imageStore(out_color, ivec2(pos), color);
}