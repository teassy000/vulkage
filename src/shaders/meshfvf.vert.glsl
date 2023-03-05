#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require


layout(location = 0) in vec3 position;
layout(location = 1) in uvec4 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 color;


void main()
{

    vec3 norm = vec3(normal.x, normal.y, normal.z) / 127.0 - 1;
    vec2 uv = uv;

    gl_Position = vec4(position + vec3(0, 0, 0.5), 1.0);

    color = vec4(norm, 1.0);
}
