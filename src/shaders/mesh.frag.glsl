#version 450

layout(location = 0) in vec4 color;
layout(location = 0) out vec4 outputColor;

void main()
{
    outputColor = color; // vec4(0.3f, 0.1f, 0.6f, 0.2f); 
}