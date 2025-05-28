#define TASKGP_SIZE 128
#define MESHGP_SIZE 64


struct Vertex
{
    float       vx, vy, vz;
    uint8_t     nx, ny, nz, nw;
    uint8_t     tx, ty, tz, tw;
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
    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;
    float screenWidth, screenHeight;
    int   enableMeshletOcclusion;
    float lodErrorThreshold;
    float probeRangeRadius;
};

struct DrawCull
{
    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float lodBase, lodStep;
    float pyramidWidth, pyramidHeight;
    float lodErrorThreshold;

    int enableCull;
    int enableLod;
    int enableSeamlessLod;
    int enableOcclusion;
    int enableMeshletOcclusion;
};

struct TransformData
{
    mat4 view;
    mat4 proj;

    mat4 cull_view;
    mat4 cull_proj;
    
    vec4 cameraPos;
    vec4 cull_cameraPos;
};

struct MeshLod
{
    uint meshletOffset;
    uint meshletCount;
    uint indexOffset;
    uint indexCount;
    float error;
};

struct Mesh
{
    vec3 center;
    float radius;

    vec3 aabbMax;
    uint vertexCount;

    vec3 aabbMin;
    uint vertexOffset;

    MeshLod lods[8];
    MeshLod seamlessLod;

    uint padding[3];
    uint lodCount;
};

struct Cluster
{
    vec3 s_c;
    float s_r;

    vec3 p_c;
    float p_r;

    float s_err;
    float p_err;

    int8_t cone_axis[3];
    int8_t cone_cutoff;

    uint dataOffset;
    uint8_t triangleCount;
    uint8_t vertexCount;
};

// Instances
struct MeshDraw
{
    vec3 pos;
    uint meshIdx;
    vec3 scale;
    uint vertexOffset; // same as mesh[meshIdx], for data locality
    vec4 orit;

    uint meshletVisibilityOffset;

    uint withAlpha;

    uint albedoTex;
    uint normalTex;
    uint specularTex;
    uint emissiveTex;
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

// terrain
struct TerrainVertex
{
    float   vx, vy, vz;
    float   tu, tv;
};


struct TerrainConstants
{
    float bx, by, bz; // base pos
    float sh, sv; // size horizon, vertical
    uint w, h; // image size
    uint tcw, tch; // tile count
};