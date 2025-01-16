# version 450


# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(push_constant) uniform blocks
{
    vec2 imageSize;
};

layout(binding = 0) uniform sampler2D albedo;
layout(binding = 1) uniform sampler2D normal;
layout(binding = 2) uniform sampler2D wPos;
layout(binding = 3) uniform sampler2D emmision;

layout(binding = 4) uniform writeonly image2D out_color;

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;
    vec2 uv = (vec2(pos) + vec2(.5)) / imageSize;

    vec4 albedo = texture(albedo, uv);
    vec4 normal = texture(normal, uv);
    vec4 wPos = texture(wPos, uv);
    vec4 emmision = texture(emmision, uv);


    if (albedo.a < 0.5)
    {
        imageStore(out_color, ivec2(pos), vec4(1.0));
        return;
    }

    imageStore(out_color, ivec2(pos), vec4(normal.rgb, 1.0));
}