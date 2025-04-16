# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_ARB_draw_instanced : require

# extension GL_GOOGLE_include_directive: require
# include "mesh_gpu.h"
# include "rc_common.h"
#include "debug_gpu.h"


layout(push_constant) uniform block
{
    ProbeDebugDrawConsts consts;
};

layout(binding = 3) uniform sampler2DArray in_cascades;

layout(location = 0) in flat ivec3 in_probeId;
layout(location = 1) in vec3 in_dir;

layout(location = 0) out vec4 outColor;

void main()
{
    ivec2 probeIdx = ivec2(in_probeId.xy);
    uint layerId = in_probeId.z;

    const uint probeSideCnt = consts.probeSideCount;
    const uint raySideCnt = consts.raySideCount;

    vec2 raySubUV = float32x3_to_oct(in_dir);
    raySubUV = raySubUV * 0.5f + 0.5f; // map to [0, 1]

    ivec2 rayIdx = ivec2((raySubUV * raySideCnt));
    vec2 pixelIdx = getRCTexelPos(consts.debugIdxType, raySideCnt, probeSideCnt, probeIdx, rayIdx);

    vec2 uv = (vec2(pixelIdx) + vec2(0.5f)) / (probeSideCnt * raySideCnt);

    vec3 color = texture(in_cascades, vec3(uv.xy, layerId)).rgb;

    outColor = vec4(color.rgb, 1.f);
}