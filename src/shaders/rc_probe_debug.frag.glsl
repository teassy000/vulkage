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
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 outColor;

void main()
{
    ivec2 probeIdx = ivec2(in_probeId.xy);
    uint layerId = in_probeId.z;

    const float probeSideCnt = float(consts.probeSideCount);
    const float raySideCnt = float(consts.raySideCount);

    ivec2 rayIdx = ivec2((in_uv * raySideCnt + 0.5f));
    ivec2 dirOrderIdx = ivec2(rayIdx * int(probeSideCnt) + probeIdx);

    vec2 uv = vec2(dirOrderIdx) / (probeSideCnt * raySideCnt);
    uv.y = 1 - uv.y;

    vec3 color = texture(in_cascades, vec3(uv.xy, layerId)).rgb;
    outColor = vec4(color.rgb, 1.f);
}