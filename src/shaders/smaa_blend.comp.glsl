# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"
#include "smaa.h"


layout(push_constant) uniform blocks
{
    vec2 imageSize;
};

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D in_weightSampler;
layout(binding = 1) uniform sampler2D in_colorSampler;
layout(binding = 2) uniform writeonly image2D out_img;

#define mad(a, b, c) (a * b + c)

vec4 SMAA_RT_METRICS = vec4(1.0 / imageSize.x, 1.0 / imageSize.y, imageSize.x, imageSize.y);

/**
 * Conditional move:
 */
void SMAAMovc(bvec2 cond, inout vec2 variable, vec2 value)
{
    if (cond.x) variable.x = value.x;
    if (cond.y) variable.y = value.y;
}

void SMAAMovc(bvec4 cond, inout vec4 variable, vec4 value)
{
    SMAAMovc(cond.xy, variable.xy, value.xy);
    SMAAMovc(cond.zw, variable.zw, value.zw);
}

void SMAABlend(vec2 _pos)
{
    vec2 uv = pos2uv(_pos, imageSize);

    // Fetch the blending weights for current pixel:
    vec4 a = vec4(0);
    a.x = texture(in_weightSampler, pos2uv(_pos + offsets[0].xy, imageSize)).a; // Right
    a.y = texture(in_weightSampler, pos2uv(_pos + offsets[1].xy, imageSize)).g; // Top
    a.wz = texture(in_weightSampler, uv).xz; // Bottom / Left

    vec4 color;
    // Is there any blending weight with a value greater than 0.0?
    if (dot(a, vec4(1.0, 1.0, 1.0, 1.0)) <= 1e-5)
    {
        color = texture(in_colorSampler, uv); // LinearSampler
    }
    else
    {
        bool h = max(a.x, a.z) > max(a.y, a.w); // max(horizontal) > max(vertical)

        // Calculate the blending offsets:
        vec4 blendingOffset = vec4(0.0, a.y, 0.0, a.w);
        vec2 blendingWeight = a.yw;
        SMAAMovc(bvec4(h, h, h, h), blendingOffset, vec4(a.x, 0.0, a.z, 0.0));
        SMAAMovc(bvec2(h, h), blendingWeight, a.xz);
        blendingWeight /= dot(blendingWeight, vec2(1.0, 1.0));

        // Calculate the texture coordinates:
        vec4 blendingCoord = mad(blendingOffset, vec4(SMAA_RT_METRICS.xy, -SMAA_RT_METRICS.xy), uv.xyxy);

        // We exploit bilinear filtering to mix current pixel with the chosen
        // neighbor:
        color = blendingWeight.x * texture(in_colorSampler,uv); // LinearSampler
        color += blendingWeight.y * texture(in_colorSampler, uv); // LinearSampler
    }

    imageStore(out_img, ivec2(_pos), color);
}

void main()
{
    vec2 pos = gl_GlobalInvocationID.xy;
    SMAABlend(pos);
}