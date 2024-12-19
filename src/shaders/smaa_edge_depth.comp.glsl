# version 450

// the impletation copied with modififacion based on https://github.com/dmnsgn/glsl-smaa
// which is based on DX version https://github.com/iryoku/smaa
// the original code is licensed under MIT

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"
#include "smaa_old.h"

layout(push_constant) uniform blocks
{
    vec2 imageSize;
};

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D in_depth;
layout(binding = 1) uniform writeonly image2D out_depth;

/**
 * Gathers current pixel, and the top-left neighbors.
 */
vec3 SMAAGatherNeighbours(vec2 _pos, vec2 _offsets[6], sampler2D tex)
{
    vec2 uv = (_pos + vec2(0.5)) / imageSize;

    float P     =   texture(tex, uv).r;
    float Pleft =   texture(tex, pos2uv(_pos + _offsets[0], imageSize)).r;
    float Ptop = texture(tex, pos2uv(_pos + _offsets[0], imageSize)).r;


    return vec3(P, Pleft, Ptop);
}

void edgeDepth(vec2 _pos)
{
    vec3 neighbors = SMAAGatherNeighbours(_pos, offsets, in_depth);
    vec2 delta = abs(neighbors.yz - neighbors.xx);
    vec2 edges = step(SMAA_DEPTH_THRESHOLD, delta);

    if (dot(edges, vec2(1.0)) > 0.0) {
        imageStore(out_depth, ivec2(_pos), vec4(edges, 0.0, 1.0));
    }
    else {
        imageStore(out_depth, ivec2(_pos), vec4(0.0, 0.0, 0.0, 1.0));
    }
}

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;

    edgeDepth(pos);
}