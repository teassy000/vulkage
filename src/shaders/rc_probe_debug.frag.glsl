# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_ARB_draw_instanced : require

# extension GL_GOOGLE_include_directive: require
# include "mesh_gpu.h"
# include "rc_common.h"


layout(push_constant) uniform block
{
    ProbeDebugConsts consts;
};

layout(binding = 3) uniform sampler2DArray in_cascades;

layout(location = 0) in flat ivec3 in_probeId;
layout(location = 0) out vec4 outColor;

void main()
{
    vec3 uv = ivec3(in_probeId);
    uv.xy /= float(consts.probeSideCount);

    vec3 color = texture(in_cascades, uv).rgb;

    outColor = vec4(color, 1.f);
}