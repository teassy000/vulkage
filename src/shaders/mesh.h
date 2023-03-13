#define TASK_SIZE 32
#define MESH_SIZE 32

struct Vertex
{
    float16_t   vx, vy, vz, vw;
    uint8_t     nx, ny, nz, nw;
    float16_t   tu, tv;
};

struct Meshlet
{
    vec4 cone;
    uint vertices[64];
    uint8_t  indices[84*3]; //maximum 84 triangles
    uint8_t  triangleCount;
    uint8_t  vertexCount;
};

struct TaskPayload
{
    uint offset;
    uint meshletIndices[TASK_SIZE];
};
