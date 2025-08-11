# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require


// vtx data
vec3 vtx_data[3] = vec3[3](
    vec3(0.0, 0.0, 0.5),
    vec3(1.0, 0.0, 0.5),
    vec3(0.0, 1.0, 0.5)
);


layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(push_constant) uniform block
{
    vec2 viewportSize; // viewport size
};

layout(binding = 0) uniform writeonly image2D out_color;
layout(binding = 1) uniform writeonly image2D out_depth;


// check if coverd by triangle
float calcDepth(vec2 _pos, vec3 _v0, vec3 _v1, vec3 _v2) {
    // Barycentric coordinates method to check if point is inside triangle
    vec2 v0v1 = _v1.xy - _v0.xy;
    vec2 v0v2 = _v2.xy - _v0.xy;
    vec2 v0p = _pos - _v0.xy;
    float d00 = dot(v0v1, v0v1);
    float d01 = dot(v0v1, v0v2);
    float d11 = dot(v0v2, v0v2);
    float d20 = dot(v0p, v0v1);
    float d21 = dot(v0p, v0v2);
    float denom = d00 * d11 - d01 * d01;
    if (denom == 0.0) return -1.0; // Degenerate triangle
    float u = (d11 * d20 - d01 * d21) / denom;
    float v = (d00 * d21 - d01 * d20) / denom;
    float w = 1.f - u - v;
    
    // Check if inside triangle
    if (u >= 0.0 && v >= 0.0 && w >= 0.0)
        return u * _v1.z + v * _v2.z + w * _v0.z; // Interpolate depth (z)
    else 
        return -1.0; // Not covered
}

void main()
{
    ivec2 uid = ivec2(gl_GlobalInvocationID.xy);
    if (uid.x >= int(viewportSize.x) || uid.y >= int(viewportSize.y)) {
        return; // out of bounds
    }

    vec3 pos = vec3(uid.x / viewportSize.x, uid.y / viewportSize.y, 0.f); // simple gradient pos based on x position

    float depth = calcDepth(pos.xy, vtx_data[0], vtx_data[1], vtx_data[2]);
    if (depth >= 0.f) {
        // Write the pos and depth to the images
        imageStore(out_color, uid, vec4(pos, 1.0)); // write pos
        imageStore(out_depth, uid, vec4(depth, 0, 0, 1.0)); // write depth
    }
}