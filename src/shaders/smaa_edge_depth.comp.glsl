# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require


layout(push_constant) uniform blocks
{
    vec2 imageSize;
};


#include "smaa_impl.h"

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform SMAATexture2D(in_depth);
layout(binding = 2) uniform writeonly image2D out_color;

void main()
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

    SMAADepthEdgeDetectionCS(pos, out_color, in_depth);
}