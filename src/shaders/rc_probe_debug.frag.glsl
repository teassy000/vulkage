# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_ARB_draw_instanced : require
//#extension GL_ARB_shader_draw_parameters : require

# extension GL_GOOGLE_include_directive: require
# include "mesh_gpu.h"
# include "rc_common.h"

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}