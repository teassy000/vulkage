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

    const float probeSideCnt = float(consts.probeSideCount);
    const float raySideCnt = float(consts.raySideCount);

    vec2 uv0 = float32x3_to_oct(in_dir);
    uv0 = uv0 * 0.5f + 0.5f; // map to [0, 1]


    ivec2 rayIdx = ivec2((uv0 * raySideCnt + 0.5f));
    ivec2 pixelIdx = ivec2(0u);
    switch (consts.debugIdxType)
    {
        case 0:  // probe first index
            pixelIdx = ivec2(probeIdx * int(raySideCnt) + rayIdx);
            break;
        case 1: // ray first index
            pixelIdx = ivec2(rayIdx * int(probeSideCnt) + probeIdx);
            break;
    }


    vec2 uv = vec2(pixelIdx) / (probeSideCnt * raySideCnt);

    vec3 color = texture(in_cascades, vec3(uv.xy, layerId)).rgb;

    outColor = vec4(color.rgb, 1.f);
}