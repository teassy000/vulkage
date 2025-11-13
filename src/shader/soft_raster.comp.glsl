# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

// for image load/store
# extension GL_ARB_shader_image_load_store: require

// for using uint8_t in general code
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_EXT_shader_explicit_arithmetic_types_int8: require

# extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"
#include "debug_gpu.h"

layout(local_size_x = MR_SOFT_RASTGP_SIZE, local_size_y = 1, local_size_z = 1) in;

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
    RasterMeshletPayload in_payloads [];
};

layout(binding = 6) readonly buffer TriangleCount
{
    IndirectDispatchCommand in_payloadCnt;
};

layout(binding = 7) uniform sampler2D pyramid;

layout(binding = 8) uniform writeonly image2D out_color;
layout(binding = 9, r32ui) uniform uimage2D out_uDepth;
layout(binding = 10, r32f) uniform writeonly image2D out_depth;
layout(binding = 11, r32ui) uniform uimage2D debug_image;

shared vec3 vertexClip[MESH_MAX_VTX];

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
    if (denom == 0.f) return -1.f; // Degenerate triangle
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
    uint ti = uint(gl_LocalInvocationID.x); // the triangle id to process
    uint mlti = uint(gl_WorkGroupID.x);
    uint glti = gl_GlobalInvocationID.x;

    RasterMeshletPayload payload = in_payloads[ti];

    MeshDraw md = meshDraws[payload.drawId];
    Meshlet mlt = meshlets[payload.meshletIdx];
    
    uint vertexCount = 0;
    uint triangleCount = 0;
    uint dataOffset = 0;
    {
        // use meshlet info
        Meshlet mlt = meshlets[mlti];
        vertexCount = uint(mlt.vertexCount);
        triangleCount = uint(mlt.triangleCount);
        dataOffset = mlt.dataOffset;
    }

    uint indexOffset = dataOffset + vertexCount;

    // transform vertices
    for (uint ii = ti; ii < vertexCount; ii += MR_SOFT_RASTGP_SIZE)
    {
        uint vi = meshletData[dataOffset + ii] + md.vertexOffset;

        vec3 pos = vec3(vertices[vi].vx, vertices[vi].vy, vertices[vi].vz);
        vec3 norm = vec3(int(vertices[vi].nx), int(vertices[vi].ny), int(vertices[vi].nz)) / 127.0 - 1.0;
        vec4 tan = vec4(int(vertices[vi].tx), int(vertices[vi].ty), int(vertices[vi].tz), int(vertices[vi].tw)) / 127.0 - 1.0;
        vec2 uv = vec2(vertices[vi].tu, vertices[vi].tv);

        vec3 wPos = rotateQuat(pos, md.orit) * md.scale + md.pos;

        vec4 result = trans.proj * trans.view * vec4(wPos, 1.0);

        
        vertexClip[ii].xyz = result.xyz / result.w;
    }

    barrier();

    // process triangles
    for (uint ii = ti; ii < triangleCount; ii += MR_SOFT_RASTGP_SIZE)
    {
        // read the mask to check if triangle is visiable
        uint64_t triBit = (payload.sr_bitmask & (1ul << ii));

        if (triBit == 0) {
            continue; // not visiable, skip
        }

        uint offset = indexOffset * 4 + ii * 3; // x4 for uint8_t

        uint idx0 = uint(meshletData8[offset + 0]);
        uint idx1 = uint(meshletData8[offset + 1]);
        uint idx2 = uint(meshletData8[offset + 2]);

        vec3 pa = vertexClip[idx0].xyz;
        vec3 pb = vertexClip[idx1].xyz;
        vec3 pc = vertexClip[idx2].xyz;

        // transform to screen space
        vec2 p0_ndc = (pa.xy * 0.5 + vec2(0.5));
        vec2 p1_ndc = (pb.xy * 0.5 + vec2(0.5));
        vec2 p2_ndc = (pc.xy * 0.5 + vec2(0.5));

        // convert to pixel space
        vec2 p0_sc = p0_ndc * viewportSize;
        vec2 p1_sc = p1_ndc * viewportSize;
        vec2 p2_sc = p2_ndc * viewportSize;

        float newDepth = pa.z;

        if(newDepth < 0.f) {
            continue; // behind near plane
        }

        uint new_ud = depthToComparableUint(newDepth);
        uint old_ud = imageLoad(out_uDepth, ivec2(p1_sc)).r;

        if (new_ud <= old_ud){
            continue; // not closer
        }

        uint mhash = hash(glti);
        vec4 hashCol = vec4(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255), 255) / 255.0;

        uint prev = imageAtomicMax(out_uDepth, ivec2(p0_sc), new_ud);
        if(prev < new_ud)
        {
            uint uDepth = imageLoad(out_uDepth, ivec2(p0_sc)).r; // Load the current depth value again
            if (uDepth == new_ud)
            {
                vec3 col = hashCol.rgb; // simple color based on triangle id
                //vec3 col = vec3(float(ti) / 1000.f);
                float currDepth = uintToDepth(uDepth);
                float newDepth = uintToDepth(new_ud);
                imageStore(out_color, ivec2(p0_sc), vec4(col, 1.0)); // write uv
                imageStore(out_depth, ivec2(p0_sc), vec4(currDepth, 0.0, 0.0, 1.0)); // write depth
                imageStore(debug_image, ivec2(p0_sc), uvec4(glti, 0, 0, 0)); // write debug info
            }
        }
    }
}