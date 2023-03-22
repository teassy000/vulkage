#version 450

#extension GL_ARB_shader_draw_parameters: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_mesh_shader: require


#extension GL_GOOGLE_include_directive: require


#include "mesh.h"

#define DEBUG 0


layout(local_size_x = MESHGP_SIZE, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices= 64, max_primitives = 84) out;

layout(push_constant) uniform block 
{
    Globals globals;
};

layout(binding = 2) readonly buffer MeshDraws 
{
    MeshDraw meshDraws[];
};

layout(binding = 3) readonly buffer Meshlets
{
    Meshlet meshlets[];
};

layout(binding = 4) readonly buffer MeshletData
{
    uint meshletData[];
};

layout(binding = 4) readonly buffer MeshletData8
{
    uint8_t meshletData8[];
};

layout(binding = 5) readonly buffer Vertices
{
    Vertex vertices[];
};



layout(location = 0) out vec4 color[];

taskPayloadSharedEXT TaskPayload payload;

uint hash(uint a)
{
   a = (a+0x7ed55d16) + (a<<12);
   a = (a^0xc761c23c) ^ (a>>19);
   a = (a+0x165667b1) + (a<<5);
   a = (a+0xd3a2646c) ^ (a<<9);
   a = (a+0xfd7046c5) + (a<<3);
   a = (a^0xb55a4f09) ^ (a>>16);
   return a;
}

void main()
{
    uint ti = gl_LocalInvocationID.x;
    uint mi = payload.meshletIndices[gl_WorkGroupID.x];

    MeshDraw meshDraw = meshDraws[payload.drawId];

    uint vertexCount = uint(meshlets[mi].vertexCount); 
    uint triangleCount = uint(meshlets[mi].triangleCount); 

    SetMeshOutputsEXT( vertexCount, triangleCount);

    uint dataOffset = meshlets[mi].dataOffset;
    uint vertexOffset = dataOffset;
    uint indexOffset = dataOffset + vertexCount;

#if DEBUG
    uint mhash = hash(mi);
    vec3 mcolor = vec3(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255)) / 255.0;
#endif

    // TODO: maybe remove the branch would help
    for(uint i = ti; i < uint(meshlets[mi].vertexCount); i += MESHGP_SIZE )
    {
        uint vi = meshletData[vertexOffset + i] + meshDraw.vertexOffset;
    
        vec3 pos = vec3(vertices[vi].vx, vertices[vi].vy, vertices[vi].vz);
        vec3 norm = vec3(int(vertices[vi].nx), int(vertices[vi].ny), int(vertices[vi].nz)) / 127.0 - 1.0;
        vec2 uv = vec2(vertices[vi].tu, vertices[vi].tv);

        vec3 result = vec3(rotateQuat( pos, meshDraw.orit) * meshDraw.scale + meshDraw.pos);

        gl_MeshVerticesEXT[i].gl_Position = globals.projection * vec4(result, 1.0);

#if DEBUG
        color[i] = vec4(mcolor, 1.0);
#else
        color[i] = vec4(norm * 0.5 + vec3(0.5), 1.0);
#endif
    }

    for(uint i = ti; i < uint(meshlets[mi].triangleCount); i += MESHGP_SIZE )
    {
        uint offset = indexOffset * 4 + i * 3;

        uint idx0 = uint(meshletData8[ offset + 0]);
        uint idx1 = uint(meshletData8[ offset + 1]);
        uint idx2 = uint(meshletData8[ offset + 2]);
        
        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(idx0, idx1, idx2);
    }
}