#version 450

#extension GL_ARB_shader_draw_parameters: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "math.h"

layout(push_constant) uniform block 
{
    Globals globals;
};

layout(binding = 0) readonly buffer DrawCommands 
{
    MeshDrawCommand drawCmds[];
};

layout(binding = 1) readonly buffer Vertices
{
    Vertex vertices[];
};

layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws[];
};

layout(binding = 3) readonly uniform Transform 
{
    TransformData trans;
};

layout(location = 0) out flat uint out_drawId;
layout(location = 1) out vec3 out_wPos;
layout(location = 2) out vec3 out_norm;
layout(location = 3) out vec4 out_tan;
layout(location = 4) out vec2 out_uv;

void main()
{
    uint drawId = drawCmds[gl_DrawIDARB].drawId;
    
    MeshDraw meshDraw = meshDraws[drawId];

    vec3 pos = vec3(vertices[gl_VertexIndex].vx, vertices[gl_VertexIndex].vy, vertices[gl_VertexIndex].vz);
    vec3 norm = vec3(int(vertices[gl_VertexIndex].nx), int(vertices[gl_VertexIndex].ny), int(vertices[gl_VertexIndex].nz)) / 127.0 - 1;
    vec4 tan = vec4(int(vertices[gl_VertexIndex].tx), int(vertices[gl_VertexIndex].ty), int(vertices[gl_VertexIndex].tz), int(vertices[gl_VertexIndex].tw)) / 127.0 - 1;
    vec2 uv = vec2(vertices[gl_VertexIndex].tu, vertices[gl_VertexIndex].tv);

    norm = rotateQuat(norm, meshDraw.orit);
    vec3 result = vec3(rotateQuat(pos, meshDraw.orit) * meshDraw.scale + meshDraw.pos);

    gl_Position = trans.proj * trans.view * vec4(result, 1.0);

    out_drawId = drawId;
    out_wPos = result;
    out_norm = norm;
    out_tan = tan;
    out_uv = uv;
}
