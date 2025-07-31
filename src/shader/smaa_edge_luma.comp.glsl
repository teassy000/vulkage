# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require


layout(push_constant) uniform blocks
{
    vec2 imageSize;
};


#include "smaa_impl.h"

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform SMAATexture2D(in_color); // point sampler
//layout(binding = 1) uniform SMAATexture2D(in_predication);
layout(binding = 1) uniform writeonly image2D out_edge;


// this just a copy of SMAALumaEdgeDetectionCS to make sure all innovation in the same layout.
// thus write to out_edge no matter edge is valid or not.

/**
 * Luma Edge Detection
 *
 * IMPORTANT NOTICE: luma edge detection requires gamma-corrected colors, and
 * thus '_colorTex' should be a non-sRGB texture.
 */
float2 SMAALumaEdgeDetectionCSInno(int2 _pos, SMAATexture2D(_colorTex))
{
    float2 texcoord = _pos.xy;
    // account for pixel center
    texcoord += float2(0.5, 0.5);
    texcoord *= SMAA_RT_METRICS.xy;

    float4 offset[3];
    offset[0] = mad(SMAA_RT_METRICS.xyxy, float4(-1.0, 0.0, 0.0, API_V_DIR(-1.0)), texcoord.xyxy);
    offset[1] = mad(SMAA_RT_METRICS.xyxy, float4(1.0, 0.0, 0.0, API_V_DIR(1.0)), texcoord.xyxy);
    offset[2] = mad(SMAA_RT_METRICS.xyxy, float4(-2.0, 0.0, 0.0, API_V_DIR(-2.0)), texcoord.xyxy);

    // Calculate the threshold:
#if SMAA_PREDICATION
    float2 threshold = SMAACalculatePredicatedThreshold(texcoord, offset[0], SMAATexturePass2D(predicationTex));
#else  // SMAA_PREDICATION
    float2 threshold = float2(SMAA_THRESHOLD, SMAA_THRESHOLD);
#endif  // SMAA_PREDICATION

    // Calculate lumas:
    float3 weights = float3(0.2126, 0.7152, 0.0722);
    float L = dot(SMAASamplePoint(_colorTex, texcoord).rgb, weights);

    float Lleft = dot(SMAASamplePoint(_colorTex, offset[0].xy).rgb, weights);
    float Ltop = dot(SMAASamplePoint(_colorTex, offset[0].zw).rgb, weights);

    // We do the usual threshold:
    float4 delta;
    delta.xy = abs(L - float2(Lleft, Ltop));
    float2 edges = step(threshold, delta.xy);

    // Then discard if there is no edge:
    if (dot(edges, float2(1.0, 1.0)) < 1e-5)
    {
        return float2(0.0);
    }

    // Calculate right and bottom deltas:
    float Lright = dot(SMAASamplePoint(_colorTex, offset[1].xy).rgb, weights);
    float Lbottom = dot(SMAASamplePoint(_colorTex, offset[1].zw).rgb, weights);
    delta.zw = abs(L - float2(Lright, Lbottom));

    // Calculate the maximum delta in the direct neighborhood:
    float2 maxDelta = max(delta.xy, delta.zw);

    // Calculate left-left and top-top deltas:
    float Lleftleft = dot(SMAASamplePoint(_colorTex, offset[2].xy).rgb, weights);
    float Ltoptop = dot(SMAASamplePoint(_colorTex, offset[2].zw).rgb, weights);
    delta.zw = abs(float2(Lleft, Ltop) - float2(Lleftleft, Ltoptop));

    // Calculate the final maximum delta:
    maxDelta = max(maxDelta.xy, delta.zw);
    float finalDelta = max(maxDelta.x, maxDelta.y);

    // Local contrast adaptation:
    edges.xy *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy);

    return edges;
}


void main()
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

#if SMAA_PREDICATION
    SMAALumaEdgeDetectionCS(pos, out_edge, in_color, in_predication);
#else
    vec2 edge = SMAALumaEdgeDetectionCSInno(pos, in_color);
    imageStore(out_edge, pos, vec4(edge.xy, 0.0, 0.0));
#endif // SMAA_PREDICATION
}