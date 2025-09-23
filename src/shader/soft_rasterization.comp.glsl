# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require
# extension GL_ARB_shader_image_load_store: require

#include "mesh_gpu.h"

// vtx data
vec3 vtx_data[] = vec3[6](
    vec3(0.0, 0.0, 0.5),
    vec3(1.0, 0.0, 0.5),
    vec3(0.0, 1.0, 0.5),
    vec3(0.5, 0.0, 0.4),
    vec3(0.0, 1.0, 0.4),
    vec3(1.0, 1.0, 0.4)
);


layout(local_size_x = MR_SOFT_RASTGP_SIZE, local_size_y = MR_SOFT_RAST_TILE_SIZE, local_size_z = MR_SOFT_RAST_TILE_SIZE) in;

layout(push_constant) uniform block
{
    vec2 viewportSize; // viewport size
};

layout(binding = 0) readonly buffer VertexData
{
    vec3 vertex_data[];
};

layout(binding = 1) readonly buffer TriangleData
{
    uvec3 tri_data[];
};

layout(binding = 2) uniform sampler2D pyramid;

layout(binding = 3) uniform writeonly image2D out_color;
layout(binding = 4, r32ui) uniform uimage2D out_uDepth;
layout(binding = 5, r32f) uniform writeonly image2D out_depth;

// =========================================
// depth conversion functions===============
// =========================================
uint depthToComparableUint(float _d) {
    // depth in float is in range [0, 1]
    // Inverse-Z: near => 0, far => 1
    // for positive depth values, floatBitsToUint perserves ordering.
    return floatBitsToUint(_d);
}

float uintToDepth(uint _u) {
    return uintBitsToFloat(_u);
}
// ==========================================

// check if coverd by triangle
float calcDepth(vec2 _pos, vec3 _v0, vec3 _v1, vec3 _v2) {
    // Barycentric coordinates method to check if point is inside triangle
    vec2 v0v1 = _v1.xy - _v0.xy;
    vec2 v0v2 = _v2.xy - _v0.xy;
    vec2 v0p = _pos - _v0.xy;
    float d00 = dot(v0v1, v0v1);
    float d01 = dot(v0v1, v0v2);
    float d11 = dot(v0v2, v0v2);
    float d20 = dot(v0p, v0v1);
    float d21 = dot(v0p, v0v2);
    float denom = d00 * d11 - d01 * d01;
    if (denom == 0.0) return -1.f; // Degenerate triangle
    float u = (d11 * d20 - d01 * d21) / denom;
    float v = (d00 * d21 - d01 * d20) / denom;
    float w = 1.f - u - v;
    
    // Check if inside triangle
    if (u >= 0.0 && v >= 0.0 && w >= 0.0)
        return u * _v1.z + v * _v2.z + w * _v0.z; // Interpolate depth (z)
    else 
        return -1.f; // Not covered
}

void main()
{
    ivec2 pid = ivec2(gl_GlobalInvocationID.xy); // the pixel pos

    if (pid.x >= int(viewportSize.x) || pid.y >= int(viewportSize.y)) {
        return; // out of bounds
    }

    uint zid = uint(gl_GlobalInvocationID.z);
    vec3 pos = vec3(pid.x / viewportSize.x, pid.y / viewportSize.y, 0.f); // simple gradient pos based on x position

    uint vid0 = tri_data[zid].x;
    uint vid1 = tri_data[zid].y;
    uint vid2 = tri_data[zid].z;

    float newDepth = calcDepth(pos.xy, vtx_data[vid0], vtx_data[vid1], vtx_data[vid2]);

    if (newDepth < 0.f) {
        return; // not covered by triangle
    }

    uint newUdepth = depthToComparableUint(newDepth);
    uint oldUdepth = imageLoad(out_uDepth, pid).r; // Load the current depth value

    if (newUdepth <= oldUdepth) {
        return; // no need to update if new depth is not greater
    }

    // try to update the depth
    uint perv = imageAtomicMax(out_uDepth, pid, newUdepth);

    if (perv < newUdepth)
    {
        uint curr = imageLoad(out_uDepth, pid).r; // Load the current depth value again
        if (curr == newUdepth)
        {
            vec3 col = zid == 0 ? vec3(pos.xy, 1.0) : vec3(pos.xy, 0.0); // color based on z id
            float currDepth = uintToDepth(curr);
            imageStore(out_color, pid, vec4(col, 1.0)); // write pos
            imageStore(out_depth, pid, vec4(currDepth, 0.0, 0.0, 1.0)); // write depth
        }
    }
}