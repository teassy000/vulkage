# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

// for image load/store
# extension GL_ARB_shader_image_load_store: require

// for using uint8_t in general code
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_EXT_shader_explicit_arithmetic_types_int8: require

# extension GL_GOOGLE_include_directive: require


#define DEBUG 0

#include "mesh_gpu.h"
#include "math.h"
#include "debug_gpu.h"

layout(local_size_x = MR_SOFT_RASTGP_SIZE, local_size_y = MR_SOFT_RAST_TILE_SIZE, local_size_z = MR_SOFT_RAST_TILE_SIZE) in;

// vtx data
vec3 vtx_data[] = vec3[6](
    vec3(0.0, 0.0, 0.5),
    vec3(1.0, 0.0, 0.5),
    vec3(0.0, 1.0, 0.5),
    vec3(0.5, 0.0, 0.4),
    vec3(0.0, 1.0, 0.4),
    vec3(1.0, 1.0, 0.4)
);

layout(push_constant) uniform block
{
    vec2 viewportSize; // viewport size
};

// readonly
layout(binding = 0) readonly buffer MeshDraws
{
    MeshDraw meshDraws [];
};

layout(binding = 1) readonly uniform Transform
{
    TransformData trans;
};

layout(binding = 2) readonly buffer Vertices
{
    Vertex vertices [];
};

layout(binding = 3) readonly buffer Meshlets
{
    Meshlet meshlets [];
};

layout(binding = 4) readonly buffer MeshletData
{
    uint meshletData [];
};

layout(binding = 4) readonly buffer MeshletData8
{
    uint8_t meshletData8 [];
};


layout(binding = 5) readonly buffer TrianglePayloads
{
    TrianglePayload tri_data [];
};

layout(binding = 6) readonly buffer TriangleCount
{
    IndirectDispatchCommand indirectCmdCount;
};

layout(binding = 7) uniform sampler2D pyramid;

layout(binding = 8) uniform writeonly image2D out_color;
layout(binding = 9, r32ui) uniform uimage2D out_uDepth;
layout(binding = 10, r32f) uniform writeonly image2D out_depth;

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
    ivec2 pid = ivec2(gl_GlobalInvocationID.yz); // the pixel position
    uint zid = uint(gl_GlobalInvocationID.x); // the triangle id to process

    if (pid.x >= int(viewportSize.x) || pid.y >= int(viewportSize.y)) {
        return; // out of bounds
    }

    if(indirectCmdCount.count <= zid) {
        return; // out of bounds
    }

    /*
    if (zid >= 2)
        return; // only process 2 triangles for debug
    */
    vec2 uv = vec2(pid.x / viewportSize.x, pid.y / viewportSize.y); // simple gradient uv based on x position

#if DEBUG

    uint tid = zid;
    // debug: use the interpolated depth of the triangle
    uint baseId = zid * 3;
    float newDepth = calcDepth(uv, vtx_data[baseId + 0], vtx_data[baseId + 1], vtx_data[baseId + 2]);
#else
    TrianglePayload tri = tri_data[zid];

    MeshDraw md = meshDraws[tri.drawId];
    Meshlet mlt = meshlets[tri.meshletIdx];
    uint tid = tri.triIdx;
    uint baseIdxOffset = mlt.vertexCount + mlt.dataOffset;
    uint baseIdxOffset8 = baseIdxOffset * 4 + tid * 3; // in bytes
    // calculate vertex ids of the triangle
    uvec3 idxes = uvec3(
        uint(meshletData8[baseIdxOffset8 + 0]),
        uint(meshletData8[baseIdxOffset8 + 1]),
        uint(meshletData8[baseIdxOffset8 + 2])
    );

    // get vertex ids
    uint vid0 = meshletData[mlt.dataOffset + idxes.x] + md.vertexOffset;
    uint vid1 = meshletData[mlt.dataOffset + idxes.y] + md.vertexOffset;
    uint vid2 = meshletData[mlt.dataOffset + idxes.z] + md.vertexOffset;

    // get vertices
    Vertex v0 = vertices[vid0];
    Vertex v1 = vertices[vid1];
    Vertex v2 = vertices[vid2];

    // uv
    vec3 p0 = vec3(v0.vx, v0.vy, v0.vz);
    vec3 p1 = vec3(v1.vx, v1.vy, v1.vz);
    vec3 p2 = vec3(v2.vx, v2.vy, v2.vz);

    // transform
    vec3 wp0 = rotateQuat(p0, md.orit) * md.scale + md.pos;
    vec3 wp1 = rotateQuat(p1, md.orit) * md.scale + md.pos;
    vec3 wp2 = rotateQuat(p2, md.orit) * md.scale + md.pos;

    vec4 tp0 = (trans.proj * trans.view * vec4(wp0, 1.0));
    vec4 tp1 = (trans.proj * trans.view * vec4(wp1, 1.0));
    vec4 tp2 = (trans.proj * trans.view * vec4(wp2, 1.0));

    // perspective divide to NDC
    vec3 ndc0 = tp0.xyz / tp0.w;
    vec3 ndc1 = tp1.xyz / tp1.w;
    vec3 ndc2 = tp2.xyz / tp2.w;

    // flip y for screen space
    ndc0.y = -ndc0.y;
    ndc1.y = -ndc1.y;
    ndc2.y = -ndc2.y;

    // to screen space [0.f, 1.f], z is ndc.z with inverse-z projected
    vec3 sp0 = vec3(ndc0.xy * 0.5 + 0.5, ndc0.z);
    vec3 sp1 = vec3(ndc1.xy * 0.5 + 0.5, ndc1.z);
    vec3 sp2 = vec3(ndc2.xy * 0.5 + 0.5, ndc2.z);
    
    // depth
    float newDepth = calcDepth(uv, sp0, sp1, sp2);

#endif
    if (newDepth < 0.f) {
        return; // not covered
    }

    uint newUdepth = depthToComparableUint(newDepth);
    uint oldUdepth = imageLoad(out_uDepth, pid).r; // Load the current depth value

    if (newUdepth <= oldUdepth){
        return; // not closer
    }

    uint mhash = hash(tid);
    vec4 hashCol = vec4(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255), 255) / 255.0;

    // try to update the depth
    uint prev = imageAtomicMax(out_uDepth, pid, newUdepth);

    if (prev < newUdepth)
    {
        uint uDepth = imageLoad(out_uDepth, pid).r; // Load the current depth value again
        if (uDepth == newUdepth)
        {
            //vec3 col = hashCol.rgb; // simple color based on triangle id

            vec3 col = vec3(float(tid) / 100.f);
            float currDepth = uintToDepth(uDepth);
            float newDepth = uintToDepth(newUdepth);
            imageStore(out_color, pid, vec4(col, 1.0)); // write uv
            imageStore(out_depth, pid, vec4(currDepth, 0.0, 0.0, 1.0)); // write depth
        }
    }
}