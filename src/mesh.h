#pragma once

struct Vertex
{
    uint16_t    vx, vy, vz, vw;
    uint8_t     nx, ny, nz, nw;
    uint16_t    tu, tv;
};

struct Meshlet
{
    uint32_t vertices[64];
    uint8_t  indices[84 * 3]; //this is 84 triangles
    uint8_t  triangleCount;
    uint8_t  vertexCount;
};

struct Mesh
{
    std::vector<Vertex>     vertices;
    std::vector<uint32_t>   indices;
    std::vector<Meshlet>    meshlets;
};
