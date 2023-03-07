struct Vertex
{
    float16_t   vx, vy, vz, vw;
    uint8_t     nx, ny, nz, nw;
    float16_t   tu, tv;
};

struct Meshlet
{
    uint vertices[64];
    uint8_t  indices[84*3]; //maximum 84 triangles
    uint8_t  triangleCount;
    uint8_t  vertexCount;
};
