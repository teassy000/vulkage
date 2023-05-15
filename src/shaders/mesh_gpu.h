#define TASKGP_SIZE 128 
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
    float radius;
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

    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;
    float screenWidth, screenHeight;
    int   enableMeshletOcclusion;
};

struct MeshDrawCull
{
    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float lodBase, lodStep;
    float pyramidWidth, pyramidHeight;

    int enableCull;
    int enableLod;
    int enableOcclusion;
    int enableMeshletOcclusion;
};

struct TransformData
{
    mat4 view;
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

    float lodDistance[8];
    MeshLod lods[8];
};

// Instances
struct MeshDraw
{
    vec3 pos;
    float scale;
    vec4 orit;

    uint meshIdx;
    uint vertexOffset; // same as mesh[meshIdx], for data locality
    uint meshletVisibilityOffset;
};

struct MeshTaskCommand
{
    uint drawId;
    uint taskOffset;
    uint taskCount;
    uint lateDrawVisibility;
    uint meshletVisibilityOffset;
};

struct MeshDrawCommand
{
    uint    drawId;
    uint    taskOffset;
    uint    taskCount;
    uint    lateDrawVisibility;

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
