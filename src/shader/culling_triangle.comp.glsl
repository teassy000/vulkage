#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

// for using uint8_t in general code
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_EXT_shader_explicit_arithmetic_types_int8: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"

layout(local_size_x = MR_TRIANGLEGP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(constant_id = 1) const bool ALPHA_PASS = false;
layout(constant_id = 2) const bool SEAMLESS_LOD = false;

layout(push_constant) uniform block 
{
    vec2 rt_sz;
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
layout(binding = 7) buffer OutTriangles
{
    TrianglePayload out_tri [];
};

layout(binding = 8) buffer OutCounts
{
    uint out_counts [2];
};

shared vec3 vertexClip[MESH_MAX_VTX];

void main()
{
    // each workgroup.y process one triangle/vertex
    uint ti = gl_WorkGroupID.y;

    // each workgroup process MR_TRIANGLEGP_SIZE meshlets
    uint gid = gl_WorkGroupID.x * gl_WorkGroupSize.x + gl_LocalInvocationID.x;

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

    // transform vertices
    for (uint i = ti; i < vertexCount; i += MR_TRIANGLEGP_SIZE)
    {
        uint vi = meshletData[vertexOffset + i] + meshDraw.vertexOffset;

        vec3 pos = vec3(vertices[vi].vx, vertices[vi].vy, vertices[vi].vz);
        vec3 norm = vec3(int(vertices[vi].nx), int(vertices[vi].ny), int(vertices[vi].nz)) / 127.0 - 1.0;
        vec4 tan = vec4(int(vertices[vi].tx), int(vertices[vi].ty), int(vertices[vi].tz), int(vertices[vi].tw)) / 127.0 - 1.0;
        vec2 uv = vec2(vertices[vi].tu, vertices[vi].tv);

        vec3 wPos = rotateQuat(pos, meshDraw.orit) * meshDraw.scale + meshDraw.pos;

        vec4 result = trans.proj * trans.view * vec4(wPos, 1.0);

        norm = rotateQuat(norm, meshDraw.orit);

        vertexClip[i] = vec3(result.xy/result.w, result.w);
    }

    // make sure all vertex are transformed
    barrier();

    // cull triangles
    for (uint i = ti; i < triangleCount; i += MR_TRIANGLEGP_SIZE)
    {
        uint offset = indexOffset * 4 + i * 3; // x4 for uint8_t

        uint idx0 = uint(meshletData8[offset + 0]);
        uint idx1 = uint(meshletData8[offset + 1]);
        uint idx2 = uint(meshletData8[offset + 2]);

        bool culled = false;

        vec2 pa = vertexClip[idx0].xy;
        vec2 pb = vertexClip[idx1].xy;
        vec2 pc = vertexClip[idx2].xy;

        vec2 eb = pb - pa;
        vec2 ec = pc - pa;

        culled = culled || (eb.x * ec.y >= eb.y * ec.x);

        vec2 bmin = (min(pa, min(pb, pc)) * 0.5 + vec2(0.5)) * rt_sz;
        vec2 bmax = (max(pa, max(pb, pc)) * 0.5 + vec2(0.5)) * rt_sz;
        float sbprec = 1.0/ 256.0;

        culled = culled || (round(bmin.x - sbprec) == round(bmax.x - sbprec) || round(bmin.y - sbprec) == round(bmax.y - sbprec));
        
        culled = culled && (vertexClip[idx0].z > 0.0 && vertexClip[idx1].z > 0.0 && vertexClip[idx2].z > 0.0);

        culled = culled && !(meshDraw.withAlpha > 0);

        if(!culled)
        {
            uint triBase = atomicAdd(out_counts[0], 1);
            out_tri[triBase].drawId = drawId;
            out_tri[triBase].meshletIdx = mi;
            out_tri[triBase].v0 = meshletData8[offset + 0];
            out_tri[triBase].v1 = meshletData8[offset + 1];
            out_tri[triBase].v2 = meshletData8[offset + 2];
        }
    }
}

