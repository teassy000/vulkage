# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require

# include "mesh_gpu.h"
# include "rc_common.h"


layout(push_constant) uniform block
{
    RadianceCascadesConfig config;
};

layout(binding = 3) readonly uniform Transform
{
    TransformData trans;
};

layout(location = 0) out vec4 out_dummy;
layout(location = 1) out vec4 out_abledo;

void main()
{
    out_abledo = vec4(1.f, 1.f, 0.f, 1.f);
}