# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require

layout(push_constant) uniform blocks
{
    vec2 imageSize;
    vec4 subsampleIndices;
};

#include "smaa_impl.h"

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform SMAATexture2D(in_edge);
layout(binding = 1) uniform SMAATexture2D(in_area);
layout(binding = 2) uniform SMAATexture2D(in_search);

layout(binding = 3) uniform writeonly image2D out_img;

void main() 
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

    SMAABlendingWeightCalculationCS(pos, out_img, in_edge, in_area, in_search, subsampleIndices);
}
