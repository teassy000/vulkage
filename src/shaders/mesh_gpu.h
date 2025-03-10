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
    float sceneRadius;
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
    vec3 cameraPos;
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

    vec3 aabbMin;
    uint vertexOffset;

    vec3 aabbMax;
    uint lodCount;

    MeshLod lods[8];
    MeshLod seamlessLod;
};

struct LodBounds
{
    vec3 center;
    float radius;
    float error;
    uint lod;
};

struct Cluster
{
    LodBounds self;
    LodBounds parent;

    uint dataOffset;
    uint8_t triangleCount;
    uint8_t vertexCount;
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