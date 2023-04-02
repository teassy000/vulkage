#version 450

#extension GL_ARB_shader_draw_parameters: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_ARB_shader_draw_parameters : require

#extension GL_GOOGLE_include_directive: require

#include "mesh.h"
#include "math.h"

layout(push_constant) uniform block 
{
    Globals globals;
};

layout(binding = 0) readonly buffer DrawCommands 
{
    MeshDrawCommand drawCmds[];
};

layout(binding = 1) readonly buffer MeshDraws
{
    MeshDraw meshDraws[];
};

layout(binding = 2) readonly buffer Vertices
{
    Vertex vertices[];
};


layout(location = 0) out vec4 color;

void main()
{
    uint drawId = drawCmds[gl_DrawIDARB].drawId;
    
    MeshDraw meshDraw = meshDraws[drawId];

    vec3 pos = vec3(vertices[gl_VertexIndex].vx, vertices[gl_VertexIndex].vy, vertices[gl_VertexIndex].vz);
    vec3 norm = vec3(int(vertices[gl_VertexIndex].nx), int(vertices[gl_VertexIndex].ny), int(vertices[gl_VertexIndex].nz)) / 127.0 - 1;
    vec2 uv = vec2(vertices[gl_VertexIndex].tu, vertices[gl_VertexIndex].tv);

    vec3 result = vec3(rotateQuat(pos, meshDraw.orit) * meshDraw.scale + meshDraw.pos);

    gl_Position = globals.projection * vec4(result, 1.0);

    color = vec4(norm * 0.5 + vec3(0.5), 1.0);
}
