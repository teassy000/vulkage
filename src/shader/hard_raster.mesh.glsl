#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_mesh_shader: require

// for using uint8_t in general code
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_EXT_shader_explicit_arithmetic_types_int8: require

#extension GL_GOOGLE_include_directive: require

#include "debug_gpu.h"
#include "mesh_gpu.h"
#include "math.h"

layout(local_size_x = MESHGP_SIZE, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = MESH_MAX_VTX, max_primitives = MESH_MAX_TRI) out;

layout(push_constant) uniform block
{
    Constants consts;
};

layout(binding = 0) readonly uniform Transform
{
    TransformData trans;
};

layout(binding = 1) readonly buffer Vertices
{
    Vertex vertices [];
};

// readonly
layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws [];
};

layout(binding = 3) readonly buffer Meshlets
{
    Meshlet meshlets [];
};

layout(binding = 3) readonly buffer Clusters
{
    Cluster clusters [];
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

layout(binding = 6) readonly buffer TrianglePayloadCount
{
    IndirectDispatchCommand in_payloadCnt;
};

layout(location = 0) out flat uint out_drawId[] ;
layout(location = 1) out vec3 out_wPos[];
layout(location = 2) out vec3 out_norm[];
layout(location = 3) out vec4 out_tan[];
layout(location = 4) out vec2 out_uv[];
layout(location = 5) out flat uint out_triId[];

shared vec3 vertexClip[MESH_MAX_VTX] ;

void main()
{
    uint ti = gl_LocalInvocationID.x;
    uint mlti = gl_WorkGroupID.x;

    RasterMeshletPayload payload = in_payloads[mlti];

    MeshDraw md = meshDraws[payload.drawId];
    Meshlet mlt = meshlets[payload.meshletIdx];

    uint vertexCount = mlt.vertexCount;
    uint triangleCount = mlt.triangleCount;
    uint dataOffset = mlt.dataOffset;

    SetMeshOutputsEXT(vertexCount, triangleCount);

    // transform vertices
    for (uint ii = ti; ii < vertexCount; ii += MESHGP_SIZE)
    {
        uint vi = meshletData[dataOffset + ii] + md.vertexOffset;

        vec3 norm = vec3(int(vertices[vi].nx), int(vertices[vi].ny), int(vertices[vi].nz)) / 127.0 - 1.0;
        vec4 tan = vec4(int(vertices[vi].tx), int(vertices[vi].ty), int(vertices[vi].tz), int(vertices[vi].tw)) / 127.0 - 1.0;
        vec2 uv = vec2(vertices[vi].tu, vertices[vi].tv);

        vec3 pos = vec3(vertices[vi].vx, vertices[vi].vy, vertices[vi].vz);
        vec3 wPos = rotateQuat(pos, md.orit) * md.scale + md.pos;
        vec4 result = trans.proj * trans.view * vec4(wPos, 1.0);

        norm = rotateQuat(norm, md.orit);
        //mltiresult.xyz = result.xyz / result.w;

        gl_MeshVerticesEXT[ii].gl_Position = result;
        out_drawId[ii] = payload.meshletIdx;
        out_norm[ii] = norm;
        out_wPos[ii] = wPos;
        out_tan[ii] = tan;
        out_uv[ii] = uv;
        out_triId[ii] = payload.meshletIdx << 8 | ii;

    }

    barrier();

    uint indexOffset = dataOffset + vertexCount;
    for (uint ii = ti; ii < triangleCount; ii += MESHGP_SIZE)
    {
        uint offset = indexOffset * 4 + ii * 3; // *4 for uint8_t

        uint idx0 = uint(meshletData8[offset + 0]);
        uint idx1 = uint(meshletData8[offset + 1]);
        uint idx2 = uint(meshletData8[offset + 2]);

        gl_PrimitiveTriangleIndicesEXT[ii] = uvec3(idx0, idx1, idx2);

        // read the mask to check if triangle is visiable
        uint64_t triBit = (payload.hr_bitmask & (1ul << ii));
        gl_MeshPrimitivesEXT[ii].gl_CullPrimitiveEXT = (triBit == 0);
    }
}
