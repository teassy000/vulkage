# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require
# extension GL_ARB_shader_draw_parameters : require

# include "mesh_gpu.h"
# include "math.h"
# include "rc_common.h"

layout(push_constant) uniform block
{
    VoxelizationConfig config;
};

layout(binding = 0) readonly buffer DrawCommands
{
    MeshDrawCommand drawCmds [];
};

layout(binding = 1) readonly buffer Vertices
{
    Vertex vertices [];
};

layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws [];
};

layout(binding = 3) readonly uniform Transform
{
    TransformData trans;
};

layout(location = 0) out flat uint out_drawId;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec4 out_tangent;
layout(location = 4) out vec3 out_wpos;

// the main method is based on 

void main()
{
    uint drawId = drawCmds[gl_DrawIDARB].drawId;

    MeshDraw meshDraw = meshDraws[drawId];

    vec3 pos = vec3(vertices[gl_VertexIndex].vx, vertices[gl_VertexIndex].vy, vertices[gl_VertexIndex].vz);
    vec3 norm = vec3(int(vertices[gl_VertexIndex].nx), int(vertices[gl_VertexIndex].ny), int(vertices[gl_VertexIndex].nz)) / 127.0 - 1;
    vec4 tan = vec4(int(vertices[gl_VertexIndex].tx), int(vertices[gl_VertexIndex].ty), int(vertices[gl_VertexIndex].tz), int(vertices[gl_VertexIndex].tw)) / 127.0 - 1;
    vec2 uv = vec2(vertices[gl_VertexIndex].tu, vertices[gl_VertexIndex].tv);

    norm = rotateQuat(norm, meshDraw.orit);

    out_drawId = drawId;
    out_normal = norm;
    out_uv = uv;

    vec3 result = vec3(rotateQuat(pos, meshDraw.orit) * meshDraw.scale + meshDraw.pos);

    gl_Position = config.proj * vec4(result, 1.0);
}