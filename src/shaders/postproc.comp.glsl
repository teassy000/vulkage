# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"


layout(push_constant) uniform blocks
{
    vec2 imageSize;
};

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D in_sampler;
layout(binding = 1) uniform writeonly image2D out_img;

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;

    vec3 color = texture(in_sampler, (vec2(pos) + vec2(0.5)) / imageSize).rgb;

    // process color
    float gray = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;

    imageStore(out_img, ivec2(pos), vec4(vec3(gray), 1));
}