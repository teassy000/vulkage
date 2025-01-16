# version 450


# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(push_constant) uniform blocks
{
    vec2 imageSize;
};

layout(binding = 0) uniform sampler2D in_albedo;
layout(binding = 1) uniform sampler2D in_normal;
layout(binding = 2) uniform sampler2D in_wPos;
layout(binding = 3) uniform sampler2D in_emmision;
layout(binding = 4) uniform sampler2D in_sky;

layout(binding = 5) uniform writeonly image2D out_color;

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;
    vec2 uv = (vec2(pos) + vec2(.5)) / imageSize;

    vec4 albedo = texture(in_albedo, uv);
    vec4 normal = texture(in_normal, uv);
    vec4 wPos = texture(in_wPos, uv);
    vec4 emmision = texture(in_emmision, uv);
    vec4 sky = texture(in_sky, uv);

    vec4 color = albedo;
    if (normal.w < 0.02)
        color = sky;

    imageStore(out_color, ivec2(pos), color);
}