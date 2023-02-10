#version 450
/*
const vec3 vertices[] = 
{
   vec3(0, 0.5, 0),
   vec3(0.5, -0.5, 0),
   vec3(-0.5, -0.5, 0),
};

void main()
{
    gl_Position = vec4(vertices[gl_VertexIndex], 1.0);
}
*/

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 uv;

layout(location = 0)
out vec4 color;

void main()
{
    gl_Position = vec4(pos + vec3(0, 0, 0.5), 1.0);

    color = vec4(norm / 256.0, 1.0);
}
