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
    TrianglePayload triPayloads [];
};

layout(binding = 6) readonly buffer TrianglePayloadCount
{
    IndirectDispatchCommand triPayloadCount;
};

layout(location = 0) out flat uint out_drawId[] ;
layout(location = 1) out vec3 out_wPos[];
layout(location = 2) out vec3 out_norm[];
layout(location = 3) out vec4 out_tan[];
layout(location = 4) out vec2 out_uv[];
layout(location = 5) out flat uint out_triId[];


void main()
{
    uint ti = gl_GlobalInvocationID.x;

    if(ti >= triPayloadCount.count)
        return;

    TrianglePayload tri = triPayloads[ti];
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

    vec3 v[3];
    SetMeshOutputsEXT(3, 1);

    for (int i = 0; i < 3; i++) {
        // get global vertex ids
        uint vid = meshletData[mlt.dataOffset + idxes[i]] + md.vertexOffset;
        // get vertices
        Vertex vert = vertices[vid];
        // positions
        vec3 p = vec3(vert.vx, vert.vy, vert.vz);
        // transform
        vec3 wp = rotateQuat(p, md.orit) * md.scale + md.pos;
        v[i] = wp;

        vec4 tp = (trans.proj * trans.view * vec4(wp, 1.0));
        v[i] = tp.xyz / tp.w;

        gl_MeshVerticesEXT[i].gl_Position = vec4(v[i], 1.0);
    }

    gl_PrimitiveTriangleIndicesEXT[ti] = uvec3(0, 1, 2);

}
