# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require

# include "mesh_gpu.h"
# include "rc_common.h"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(push_constant) uniform block
{
    RadianceCascadesConfig config;
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


layout(location = 0) out flat uint out_drawId[] ;
layout(location = 1) out vec2 out_uv[];
layout(location = 2) out vec3 out_normal[];
layout(location = 3) out vec4 out_tangent[];
layout(location = 4) out vec3 out_wpos[];

// the main method is based on 

void main()
{
    vec4 v0, v1, v2;
    v0 = vec4(0.f, 0.f, 1.f, 1.f);
    v1 = vec4(1.f, 1.f, 1.f, 1.f);
    v2 = vec4(1.f, 0.f, 1.f, 1.f);

    gl_Position = v0;
    EmitVertex();

    gl_Position = v1;
    EmitVertex();

    gl_Position = v2;
    EmitVertex();

    EndPrimitive();
}