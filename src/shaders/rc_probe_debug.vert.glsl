# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_ARB_draw_instanced : require

# extension GL_GOOGLE_include_directive: require
# include "mesh_gpu.h"
# include "rc_common.h"

layout(push_constant) uniform block
{
    ProbeDebugDrawConsts consts;
};

layout(binding = 0) readonly uniform Transform
{
    TransformData trans;
};

layout(binding = 1) readonly buffer Vertices
{
    Vertex vertices [];
};

layout(binding = 2) readonly buffer ProbeDraws
{
    ProbeDraw probDraws [];
};

layout(location = 0) out flat ivec3 out_probeId;

void main()
{
    int instId = gl_InstanceIndex;
    ProbeDraw draw = probDraws[instId];
    vec3 vpos = draw.pos;
    ivec3 probeIdx = draw.idx;

    uint vi = gl_VertexIndex;
    vec3 pos = vec3(vertices[vi].vx, vertices[vi].vy, vertices[vi].vz);

    pos *= (consts.sphereRadius * .5f);
    pos += vpos;

    out_probeId = probeIdx;
    
    gl_Position = trans.proj * trans.view * vec4(pos, 1.0);
}