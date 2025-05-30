#pragma once

#include "kage_math.h"
#include <vector>

struct Vertex
{
    float       vx, vy, vz;
    uint8_t     nx, ny, nz, nw;
    uint8_t     tx, ty, tz, tw;
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
    float error;
};

struct alignas(16) Mesh
{
    vec3 center;
    float radius;

    vec3 aabbMax;
    uint32_t vertexCount;

    vec3 aabbMin;
    uint32_t vertexOffset;

    MeshLod lods[8];
    MeshLod seamlessLod;

    uint32_t padding[3];
    uint32_t lodCount;
};

struct alignas(16) LodBounds
{
    vec3 center;
    float radius;
    float error;
    uint32_t lod;
};

struct alignas(16) Cluster
{
    vec3 s_c;
    float s_r;

    vec3 p_c;
    float p_r;

    float s_err;
    float p_err;

    int8_t cone_axis[3];
    int8_t cone_cutoff;

    uint32_t dataOffset;
    uint8_t triangleCount;
    uint8_t vertexCount;
};

// store all data for meshes with same rendering properties
struct Geometry
{
    std::vector<Vertex>     vertices;
    std::vector<uint32_t>   indices;
    std::vector<Meshlet>    meshlets;
    std::vector<Cluster>    clusters;
    std::vector<uint32_t>   meshletdata;
    std::vector<Mesh>       meshes;
};

// seamless lod data
// this, essentially: nx, ny, nz in float
// are required by meshopt_simplifyWithAttributes
struct alignas(16) SeamlessVertex
{
    float px, py, pz;
    float nx, ny, nz;
    float tx, ty, tz, tw; // tangent
    float tu, tv;
};

struct alignas(16) SeamlessCluster
{
    std::vector<uint32_t>   indices;
    std::vector<uint32_t>   data;

    LodBounds self;
    LodBounds parent;

    int8_t cone_axis[3];
    int8_t cone_cutoff;

    uint32_t dataOffset;
    uint32_t vertexCount;
    uint32_t triangleCount;
};

size_t appendMeshlets(Geometry& result, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
bool appendMesh(Geometry& _result, std::vector<Vertex>& _vtxes, std::vector<uint32_t>& _idxes, bool _buildMeshlets);
bool processSeamlessMesh(Geometry& _outGeo, std::vector<SeamlessVertex>& _vertices, const size_t _idxCount);
bool loadObj(Geometry& result, const char* path, bool buildMeshlets, bool seamlessLod);