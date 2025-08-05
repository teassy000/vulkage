# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require


// vtx data
vec3 vtx_data[3] = vec3[3](
    vec3(0.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0)
);


layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(push_constant) uniform block
{
    vec2 viewportSize; // viewport size
};

layout(binding = 0) uniform writeonly image2D out_color;
layout(binding = 1) uniform writeonly image2D out_depth;


void main()
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= int(viewportSize.x) || pos.y >= int(viewportSize.y)) {
        return; // out of bounds
    }

    // Calculate the depth value based on the position
    float depth = 0.f;
    vec3 color = vec3(pos.x / viewportSize.x, pos.y / viewportSize.y, 0.f); // simple gradient color based on x position

    // Write the color and depth to the images
    imageStore(out_color, pos, vec4(color, 1.0)); // write color
    imageStore(out_depth, pos, vec4(depth, depth, depth, 1.0)); // write depth
}