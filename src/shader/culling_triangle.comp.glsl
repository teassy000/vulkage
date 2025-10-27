#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

// for using uint8_t in general code
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_EXT_shader_explicit_arithmetic_types_int8: require

#extension GL_GOOGLE_include_directive: require

#define DEBUG 1

#include "mesh_gpu.h"
#include "math.h"

layout(local_size_x = MR_TRIANGLEGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(constant_id = 0) const bool LATE = false;
layout(constant_id = 1) const bool ALPHA_PASS = false;
layout(constant_id = 2) const bool SEAMLESS_LOD = false;

layout(push_constant) uniform block 
{
    Constants consts;
};

// readonly
layout(binding = 0) readonly buffer MeshletInfo
{
    MeshletPayload payloads [];
};

layout(binding = 1) readonly buffer MeshDraws
{
    MeshDraw meshDraws [];
};

layout(binding = 2) readonly uniform Transform
{
    TransformData trans;
};

layout(binding = 3) readonly buffer Vertices
{
    Vertex vertices [];
};

layout(binding = 4) readonly buffer Meshlets
{
    Meshlet meshlets [];
};

layout(binding = 5) readonly buffer MeshletData
{
    uint meshletData [];
};

layout(binding = 5) readonly buffer MeshletData8
{
    uint8_t meshletData8[];
};

layout(binding = 6) readonly buffer MeshletCount
{
    IndirectDispatchCommand indirectCmdCount;
};

// writeonly
layout(binding = 7) buffer OutMSTriangles
{
    TrianglePayload out_hw_tri [];
};

layout(binding = 8) buffer OutSubTexelTriangles
{
    TrianglePayload out_sw_tri [];
};

layout(binding = 9) buffer OutCounts
{
    IndirectDispatchCommand outTriCnts[];
};

// pyramid
layout(binding = 10) uniform sampler2D pyramid;

shared vec3 vertexClip[MESH_MAX_VTX];

void main()
{
    // each workgroup.y process one triangle/vertex
    uint ti = gl_LocalInvocationID.x;

    // each workgroup process MR_TRIANGLEGP_SIZE meshlets
    uint gid = gl_WorkGroupID.x;

    uint count = indirectCmdCount.count;
    if (gid >= count)
        return;

    uint mi = payloads[gid].meshletIdx;
    uint drawId = payloads[gid].drawId;

    MeshDraw meshDraw = meshDraws[drawId];

    uint vertexCount = 0;
    uint triangleCount = 0;
    uint dataOffset = 0;

    /*
    if (SEAMLESS_LOD)
    {
        // use cluster info
        Cluster clt = clusters[mi];
        vertexCount = uint(clt.vertexCount);
        triangleCount = uint(clt.triangleCount);
        dataOffset = clt.dataOffset;
    }
    else // normal lod
    */
    {
        // use meshlet info
        Meshlet mlt = meshlets[mi];
        vertexCount = uint(mlt.vertexCount);
        triangleCount = uint(mlt.triangleCount);
        dataOffset = mlt.dataOffset;
    }

    uint vertexOffset = dataOffset;
    uint indexOffset = dataOffset + vertexCount;

#if DEBUG
    for (uint i = 0; i < vertexCount; i ++)
#else
    // transform vertices
    for (uint i = ti; i < vertexCount; i += MR_TRIANGLEGP_SIZE)
#endif
    {
        uint vi = meshletData[vertexOffset + i] + meshDraw.vertexOffset;

        vec3 pos = vec3(vertices[vi].vx, vertices[vi].vy, vertices[vi].vz);
        vec3 wPos = rotateQuat(pos, meshDraw.orit) * meshDraw.scale + meshDraw.pos;
        vec4 result = trans.cull_proj * trans.cull_view * vec4(wPos, 1.0);
        vertexClip[i].xyz = result.xyz / result.w;
    }

#if !DEBUG
    // make sure all vertex are transformed
    barrier();
#endif

// cull triangles

    for (uint i = ti; i < triangleCount; i += MR_TRIANGLEGP_SIZE)
    {
        uint offset = indexOffset * 4 + i * 3; // x4 for uint8_t

        uint idx0 = uint(meshletData8[offset + 0]);
        uint idx1 = uint(meshletData8[offset + 1]);
        uint idx2 = uint(meshletData8[offset + 2]);

        bool culled = false;

        vec3 pa = vertexClip[idx0].xyz;
        vec3 pb = vertexClip[idx1].xyz;
        vec3 pc = vertexClip[idx2].xyz;

        vec2 eb = pb.xy - pa.xy;
        vec2 ec = pc.xy - pa.xy;

        // back face culling in screen space
        culled = culled || (eb.x * ec.y >= eb.y * ec.x);

        // calculate the aabb of the triangle in NDC space
        vec4 aabb = vec4(
            min(pa.xy, min(pb.xy, pc.xy)) * 0.5 + vec2(0.5)
            , max(pa.xy, max(pb.xy, pc.xy)) * 0.5 + vec2(0.5));

        // cull if the triangle is too small (0.5 pixel or less)
        vec2 rt_sz = vec2(consts.screenWidth, consts.screenHeight);
        vec2 bmin = aabb.xy * rt_sz;
        vec2 bmax = aabb.zw * rt_sz;
        float sbprec = 1.0/ 256.0;

        // check if any demension is less than 0.5 pixel
        culled = culled || (round(bmin.x - sbprec) == round(bmax.x - sbprec) || round(bmin.y - sbprec) == round(bmax.y - sbprec));

        // calculate the aabb of the triangle in screen space
        float pyw = (aabb.z - aabb.x) * consts.pyramidWidth;
        float pyh = (aabb.w - aabb.y) * consts.pyramidHeight;
        float lv = floor(log2(max(pyw, pyh)));
        float zmax = max(pa.z, max(pb.z, pc.z));
        float depth = textureLod(pyramid, (aabb.xy + aabb.zw) * 0.5f, lv).r;

        // occlusion culling
        culled = culled || (zmax + sbprec < depth);

        // the culling only happen if all vertices are beyond the near plane
        culled = culled && (pa.z < 1.f && pb.z < 1.f && pc.z < 1.f);

        // don't cull triangles with alpha
        culled = culled && !(meshDraw.withAlpha > 0);

        // if a triangle's aabb is just fall into 1 pixel
        bool is_tinny = 
                (round(bmin.x - sbprec) + 1 == round(bmax.x - sbprec)) 
            &&  (round(bmin.y - sbprec) + 1 == round(bmax.y - sbprec));

        if (!culled) {
            if (is_tinny) {
                uint triBase = atomicAdd(outTriCnts[1].count, 1);
                out_sw_tri[triBase].drawId = drawId;
                out_sw_tri[triBase].meshletIdx = mi;
                out_sw_tri[triBase].triIdx = i;
            } else {
                uint triBase = atomicAdd(outTriCnts[0].count, 1);
                out_hw_tri[triBase].drawId = drawId;
                out_hw_tri[triBase].meshletIdx = mi;
                out_hw_tri[triBase].triIdx = i;
            }
        }
    }
}

