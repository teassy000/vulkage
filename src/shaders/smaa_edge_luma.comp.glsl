# version 450
// the impletation copied with modififacion based on https://github.com/dmnsgn/glsl-smaa
// which is based on DX version https://github.com/iryoku/smaa
// the original code is licensed under MIT

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

layout(binding = 0) uniform sampler2D in_color;
layout(binding = 1) uniform writeonly image2D out_luma;

void edgeLuma(vec2 _pos)
{
    vec2 threshold = vec2(SMAA_THRESHOLD);

    // Calculate lumas:
    vec3 weights = vec3(0.2126, 0.7152, 0.0722);
    float L = dot(texture(in_color, pos2uv(_pos, imageSize)).rgb, weights);

    float Lleft = dot(texture(in_color, pos2uv(_pos + offsets[0], imageSize)).rgb, weights);
    float Ltop = dot(texture(in_color, pos2uv(_pos + offsets[1], imageSize)).rgb, weights);

    // We do the usual threshold:
    vec4 delta;
    delta.xy = abs(L - vec2(Lleft, Ltop));
    vec2 edges = step(threshold, delta.xy);

    // Then discard if there is no edge:
    if (dot(edges, vec2(1.0, 1.0)) == 0.0)
    {
        imageStore(out_luma, ivec2(_pos), vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }
    else
    {
        // Calculate right and bottom deltas:
        float Lright = dot(texture(in_color, pos2uv(_pos + offsets[2], imageSize)).rgb, weights);
        float Lbottom = dot(texture(in_color, pos2uv(_pos + offsets[3], imageSize)).rgb, weights);
        delta.zw = abs(L - vec2(Lright, Lbottom));

        // Calculate the maximum delta in the direct neighborhood:
        vec2 maxDelta = max(delta.xy, delta.zw);

        // Calculate left-left and top-top deltas:
        float Lleftleft = dot(texture(in_color, pos2uv(_pos + offsets[4], imageSize)).rgb, weights);
        float Ltoptop = dot(texture(in_color, pos2uv(_pos + offsets[5], imageSize)).rgb, weights);
        delta.zw = abs(vec2(Lleft, Ltop) - vec2(Lleftleft, Ltoptop));

        // Calculate the final maximum delta:
        maxDelta = max(maxDelta.xy, delta.zw);
        float finalDelta = max(maxDelta.x, maxDelta.y);

        // Local contrast adaptation:
        edges.xy *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy);
        imageStore(out_luma, ivec2(_pos), vec4(edges, 0.0, 1.0));
    }
}

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;

    edgeLuma(pos);
}