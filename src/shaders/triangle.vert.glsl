#version 450

struct Vertex
{
    float vx, vy, vz;
    float nx, ny, nz;
    float tu, tv;
};

layout(binding = 0) buffer Vertices
{
    Vertex vertices[];
};

layout(location = 0)
out vec4 color;

void main()
{
    Vertex v = vertices[gl_VertexIndex];

    vec3 pos = vec3(v.vx, v.vy, v.vz);
    vec3 norm = vec3(v.nx, v.ny, v.nz);
    vec2 uv = vec2(v.tu, v.tv);

    gl_Position = vec4(pos + vec3(0, 0, 0.5), 1.0);


    color = vec4(norm / 256.0, 1.0);
}
