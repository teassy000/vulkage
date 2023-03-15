#define TASKGP_SIZE 32 
#define MESHGP_SIZE 32

struct Vertex
{
    float       vx, vy, vz;
    uint8_t     nx, ny, nz, nw;
    float16_t   tu, tv;
};

struct Meshlet
{
    vec4 cone;
    uint dataOffset;
    uint8_t triangleCount;
    uint8_t vertexCount;
};

struct TaskPayload
{
    uint offset;
    uint meshletIndices[TASKGP_SIZE];
};

struct Constants
{
    vec2 scale;
    vec2 offset;
};
