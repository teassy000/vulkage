#pragma once

struct Vertex
{
    float       vx, vy, vz;
    uint8_t     nx, ny, nz, nw;
    uint16_t    tu, tv;
};

struct alignas(16) Meshlet
{
    vec3 center;
    float radius;
    int8_t cone_axis[3];
    int8_t cone_cutoff;

    uint32_t dataOffset;
    uint8_t triangleCount;
    uint8_t vertexCount;
};

struct MeshLod
{
    uint32_t meshletOffset;
    uint32_t meshletCount;
    uint32_t indexOffset;
    uint32_t indexCount;
};

struct alignas(16) Mesh
{
    vec3 center;
    float   radius;

    uint32_t vertexOffset;
    uint32_t lodCount;

    float lodDistance[8];
    MeshLod lods[8];
};

struct Geometry
{
    std::vector<Vertex>     vertices;
    std::vector<uint32_t>   indices;
    std::vector<Meshlet>    meshlets;
    std::vector<uint32_t>   meshletdata;
    std::vector<Mesh>       meshes;
};

size_t appendMeshlets(Geometry& result, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
bool loadMesh(Geometry& result, const char* path, bool buildMeshlets);