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

        vec2 rt_sz = vec2(consts.screenWidth, consts.screenHeight);

        // convert to ndc space [0.f, 1.f]
        vec2 p0 = (pa.xy * 0.5 + vec2(0.5));
        vec2 p1 = (pb.xy * 0.5 + vec2(0.5));
        vec2 p2 = (pc.xy * 0.5 + vec2(0.5));

        // to screen space in pixel
        vec2 p0_rt = p0 * rt_sz;
        vec2 p1_rt = p1 * rt_sz;
        vec2 p2_rt = p2 * rt_sz;

        // coverage area in screen space with **Shoelace formula**
        float area = ((p0_rt.x - p2_rt.x) * (p1_rt.y - p0_rt.y) - (p0_rt.x - p1_rt.x) * (p2_rt.y - p0_rt.y)) * 0.5f;
        float area_abs = abs(area);

        // back face culling in screen space
        // note: here we consider clockwise triangle as front face
        culled = culled || (area >= 0.f);

        // cull if the triangle is too small (0.5 pixel or less)
        culled = culled || (area_abs < 0.5f); // 0.5 pixel area

        // occlusion culling
        // TODO: this when wrong with error, need to be fixed later
        // some triangles are culled even they are clearly visible��

        // calculate the aabb of the triangle in screen space
        vec4 aabb = vec4(min(p0.xy, min(p1.xy, p2.xy)), max(p0.xy, max(p1.xy, p2.xy)));
        aabb = clamp(aabb, vec4(0.f), vec4(1.f));
        float pyw = (aabb.z - aabb.x) * consts.pyramidWidth;
        float pyh = (aabb.w - aabb.y) * consts.pyramidHeight;
        float lv = floor(log2(max(1.f, max(pyw, pyh))));
        lv = max(lv, 0.f);
        float zmax = max(pa.z, max(pb.z, pc.z));
        float depth = textureLod(pyramid, (aabb.xy + aabb.zw) * 0.5f, lv).r;

        float sbprec = 1.f / 256.f;
        culled = culled || ((zmax + sbprec) < depth);
        


        // the culling only happen if all vertices are beyond the near plane
        culled = culled && (pa.z < 1.f && pb.z < 1.f && pc.z < 1.f);

        // don't cull triangles with alpha
        culled = culled && !(meshDraw.withAlpha > 0);

        if (!culled) {
            // if the area is less than or equal to 1 pixel, consider it as sub-texel triangle
            if (area_abs <= 1.f) {
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

