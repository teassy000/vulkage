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
    uint meshletIndices[TASKGP_SIZE];
};

struct Constants
{
    mat4 projection;
    vec3 pos;
    float scale;
    vec4 orit;
};

vec3 rotateQuat(vec3 v, vec4 q)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}
