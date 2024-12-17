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
layout(binding = 1) uniform writeonly image2D out_color;

void edgeColor(vec2 _pos)
{
    // Calculate the threshold:
    vec2 threshold = vec2(SMAA_THRESHOLD);

    // Calculate color deltas:
    vec4 delta;
    vec3 c = texture(in_color, pos2uv(_pos, imageSize)).rgb;

    vec3 cLeft = texture(in_color, pos2uv(_pos + offsets[0], imageSize)).rgb;
    vec3 t = abs(c - cLeft);
    delta.x = max(max(t.r, t.g), t.b);

    vec3 cTop = texture(in_color, pos2uv(_pos + offsets[1], imageSize)).rgb;
    t = abs(c - cTop);
    delta.y = max(max(t.r, t.g), t.b);

    // We do the usual threshold:
    vec2 edges = step(threshold, delta.xy);

    // Then discard if there is no edge:
    if (dot(edges, vec2(1.0, 1.0)) == 0.0)
    {
        imageStore(out_color, ivec2(_pos), vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }
    else
    {
        // Calculate right and bottom deltas:
        vec3 cRight = texture(in_color, pos2uv(_pos + offsets[2], imageSize)).rgb;
        t = abs(c - cRight);
        delta.z = max(max(t.r, t.g), t.b);

        vec3 cBottom = texture(in_color, pos2uv(_pos + offsets[3], imageSize)).rgb;
        t = abs(c - cBottom);
        delta.w = max(max(t.r, t.g), t.b);

        // Calculate the maximum delta in the direct neighborhood:
        vec2 maxDelta = max(delta.xy, delta.zw);

        // Calculate left-left and top-top deltas:
        vec3 cLeftLeft = texture(in_color, pos2uv(_pos + offsets[4], imageSize)).rgb;
        t = abs(c - cLeftLeft);
        delta.z = max(max(t.r, t.g), t.b);

        vec3 cTopTop = texture(in_color, pos2uv(_pos + offsets[5], imageSize)).rgb;
        t = abs(c - cTopTop);
        delta.w = max(max(t.r, t.g), t.b);

        // Calculate the final maximum delta:
        maxDelta = max(maxDelta.xy, delta.zw);
        float finalDelta = max(maxDelta.x, maxDelta.y);

        // Local contrast adaptation:
        edges.xy *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy);

        // Write the edge to the output buffer:
        imageStore(out_color, ivec2(_pos), vec4(edges, 0.0, 1.0));
    }
}

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;

    edgeColor(pos);
}