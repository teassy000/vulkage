#define TASKGP_SIZE 64
#define MESHGP_SIZE 64

struct Vertex
{
    float       vx, vy, vz;
    uint8_t     nx, ny, nz, nw;
    float16_t   tu, tv;
};

struct Meshlet
{
    vec3 center;
    float radians;
    int8_t cone_axis[3];
    int8_t cone_cutoff;
   
    uint dataOffset;
    uint8_t triangleCount;
    uint8_t vertexCount;
};

struct TaskPayload
{
    uint offset;
    uint drawId;
    uint meshletIndices[TASKGP_SIZE];
};

struct Globals
{
    mat4 projection;
    vec3 cameraPos;
};

struct MeshDrawCull
{
    mat4 view;
    float znear;
    float zfar;
    float frustum[4];
    vec3 cameraPos;
};

struct MeshLod
{
    uint meshletOffset;
    uint meshletCount;
    uint indexOffset;
    uint indexCount;
};

struct Mesh
{
    vec3 center;
    float radius;

    uint vertexOffset;
    uint lodCount;
    uint padding[2];

    float lodDistance[8];
    MeshLod lods[8];
};

struct MeshDraw
{
    vec3 pos;
    float scale;
    vec4 orit;

    uint meshIdx;
    uint vertexOffset; // same as mesh[meshIdx], for data locality
};

struct MeshDrawCommand
{
    uint    drawId;
    uint    lodIdx;

    // struct VkDrawIndexedIndirectCommand
    uint    indexCount;
    uint    instanceCount;
    uint    firstIndex;
    uint    vertexOffset;
    uint    firstInstance;
    
    // struct VkDrawMeshTasksIndirectCommandEXT
    uint    local_x;
    uint    local_y;
    uint    local_z;
};

vec3 rotateQuat(vec3 v, vec4 q)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}
