#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_mesh_shader: require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices= 64, max_primitives = 42) out;

struct Vertex
{
    float vx, vy, vz;
    uint8_t nx, ny, nz, nw;
    float tu, tv;
};

layout(binding = 0) readonly buffer Vertices
{
    Vertex vertices[];
};


struct Meshlet
{
    uint vertices[64];
    uint8_t indices[126]; //42 triangles: 42 * 3
    uint8_t indexCount;
    uint8_t vertexCount;
};

layout(binding = 1) readonly buffer Meshlets
{
    Meshlet meshlets[];
};

layout(location = 0) out vec4 color[];


void main()
{
    uint mi = gl_WorkGroupID.x;

    SetMeshOutputsEXT(uint(meshlets[mi].vertexCount), uint(meshlets[mi].indexCount) / 3);

    for(uint i = 0; i < uint(meshlets[mi].vertexCount); ++i)
    {
        uint vi = meshlets[mi].vertices[i];
    
        vec3 pos = vec3(vertices[vi].vx, vertices[vi].vy, vertices[vi].vz);
        vec3 norm = vec3(int(vertices[vi].nx), int(vertices[vi].ny), int(vertices[vi].nz)) / 127.0 - 1;
        vec2 uv = vec2(vertices[vi].tu, vertices[vi].tv);
       

        gl_MeshVerticesEXT[i].gl_Position = vec4(pos + vec3(0, 0, 0.5), 1.0);
        color[i] = vec4(norm, 1.0);
    }

    uint idx = 0;
    for(uint i = 0; i < uint(meshlets[mi].indexCount); i += 3)
    {
        uint idx0 = uint(meshlets[mi].indices[i + 0]);
        uint idx1 = uint(meshlets[mi].indices[i + 1]);
        uint idx2 = uint(meshlets[mi].indices[i + 2]);

        
        gl_PrimitiveTriangleIndicesEXT[idx++] = uvec3(idx0, idx1, idx2);
    }

}
