#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_mesh_shader: require

#extension GL_GOOGLE_include_directive: require

#include "mesh.h"

#define DEBUG 0


layout(local_size_x = MESH_SIZE, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices= 64, max_primitives = 84) out;

layout(binding = 0) readonly buffer Vertices
{
    Vertex vertices[];
};

layout(binding = 1) readonly buffer Meshlets
{
    Meshlet meshlets[];
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


    SetMeshOutputsEXT(uint(meshlets[mi].vertexCount), uint(meshlets[mi].triangleCount));

#if DEBUG
    uint mhash = hash(mi);
    vec3 mcolor = vec3(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255)) / 255.0;
#endif

    for(uint i = ti; i < uint(meshlets[mi].vertexCount); i += MESH_SIZE)
    {
        uint vi = meshlets[mi].vertices[i];
    
        vec3 pos = vec3(vertices[vi].vx, vertices[vi].vy, vertices[vi].vz);
        vec3 norm = vec3(int(vertices[vi].nx), int(vertices[vi].ny), int(vertices[vi].nz)) / 127.0 - 1.0;
        vec2 uv = vec2(vertices[vi].tu, vertices[vi].tv);
       
        gl_MeshVerticesEXT[i].gl_Position = vec4(pos + vec3(0, 0, 0.5), 1.0);

#if DEBUG
        color[i] = vec4(mcolor, 1.0);
#else
        color[i] = vec4(norm * 0.5 + vec3(0.5), 1.0);
#endif
    }


    for(uint i = ti; i < uint(meshlets[mi].triangleCount); i += MESH_SIZE )
    {
        uint idx0 = uint(meshlets[mi].indices[i * 3 + 0]);
        uint idx1 = uint(meshlets[mi].indices[i * 3 + 1]);
        uint idx2 = uint(meshlets[mi].indices[i * 3 + 2]);
        
        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(idx0, idx1, idx2);
    }
}