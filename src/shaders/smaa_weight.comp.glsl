# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require
# extension GL_GOOGLE_include_directive: require

layout(push_constant) uniform blocks
{
    uvec2 imageSize;
    vec4 subsampleIndices;
};

#include "smaa_impl.h"

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform SMAATexture2D(in_edge);
layout(binding = 1) uniform SMAATexture2D(in_edgePoint);
layout(binding = 2) uniform SMAATexture2D(in_area);
layout(binding = 3) uniform SMAATexture2D(in_search);

layout(binding = 4) uniform writeonly image2D out_img;



//-----------------------------------------------------------------------------
// Blending Weight Calculation Compute Shader (Second Pass)

float4 SMAABlendingWeightCalculationCSInno(int2 coord, SMAAWriteImage2D(blendTex)
                                        , SMAATexture2D(edgesTexPoint)
                                        , SMAATexture2D(edgesTex), SMAATexture2D(areaTex)
                                        , SMAATexture2D(searchTex), float4 subsampleIndices)
{
    float2 texcoord = coord.xy;
    // account for pixel center
    texcoord += float2(0.5, 0.5);
    texcoord *= SMAA_RT_METRICS.xy;
    float2 pixcoord = texcoord * SMAA_RT_METRICS.zw;

    float4 offset[3];
    // We will use these offsets for the searches later on (see @PSEUDO_GATHER4):
    offset[0] = mad(SMAA_RT_METRICS.xyxy, float4(-0.25, API_V_DIR(-0.125), 1.25, API_V_DIR(-0.125)), texcoord.xyxy);
    offset[1] = mad(SMAA_RT_METRICS.xyxy, float4(-0.125, API_V_DIR(-0.25), -0.125, API_V_DIR(1.25)), texcoord.xyxy);

    // And these for the searches, they indicate the ends of the loops:
    offset[2] = mad(SMAA_RT_METRICS.xxyy,
                    float4(-2.0, 2.0, API_V_DIR(-2.0), API_V_DIR(2.0)) * float(SMAA_MAX_SEARCH_STEPS),
                    float4(offset[0].xz, offset[1].yw));

    // Just pass zero for SMAA 1x, see @SUBSAMPLE_INDICES.
    float4 weights = float4(0.0, 0.0, 0.0, 0.0);

    float2 e = SMAALoad(edgesTexPoint, coord).rg;

    SMAA_BRANCH
    if (e.g > 0.0)
    { // Edge at north
#ifdef SMAA_USE_DIAG_DETECTION
        // Diagonals have both north and west edges, so searching for them in
        // one of the boundaries is enough.
        weights.rg = SMAACalculateDiagWeights(SMAATexturePass2D(edgesTex), SMAATexturePass2D(areaTex), texcoord, e, subsampleIndices);

        // We give priority to diagonals, so if we find a diagonal we skip
        // horizontal/vertical processing.
        SMAA_BRANCH
        if (weights.r == -weights.g)
        { // weights.r + weights.g == 0.0
#endif  // SMAA_USE_DIAG_DETECTION

            float2 d;

            // Find the distance to the left:
            float3 coords;
            coords.x = SMAASearchXLeft(SMAATexturePass2D(edgesTex), SMAATexturePass2D(searchTex), offset[0].xy, offset[2].x);
            coords.y = offset[1].y; // offset[1].y = texcoord.y - 0.25 * SMAA_RT_METRICS.y (@CROSSING_OFFSET)
            d.x = coords.x;

            // Now fetch the left crossing edges, two at a time using bilinear
            // filtering. Sampling at -0.25 (see @CROSSING_OFFSET) enables to
            // discern what value each edge has:
            float e1 = SMAASampleLevelZero(edgesTex, coords.xy).r;

            // Find the distance to the right:
            coords.z = SMAASearchXRight(SMAATexturePass2D(edgesTex), SMAATexturePass2D(searchTex), offset[0].zw, offset[2].y);
            d.y = coords.z;

            // We want the distances to be in pixel units (doing this here allow to
            // better interleave arithmetic and memory accesses):
            d = abs(round(mad(SMAA_RT_METRICS.zz, d, -pixcoord.xx)));

            // SMAAArea below needs a sqrt, as the areas texture is compressed
            // quadratically:
            float2 sqrt_d = sqrt(d);

            // Fetch the right crossing edges:
            float e2 = SMAASampleLevelZeroOffset(edgesTex, coords.zy, int2(1, 0)).r;

            // Ok, we know how this pattern looks like, now it is time for getting
            // the actual area:
            weights.rg = SMAAArea(SMAATexturePass2D(areaTex), sqrt_d, e1, e2, subsampleIndices.y);

            // Fix corners:
            coords.y = texcoord.y;
            SMAADetectHorizontalCornerPattern(SMAATexturePass2D(edgesTex), weights.rg, coords.xyzy, d);

#ifdef SMAA_USE_DIAG_DETECTION
        }
        else
        {
            e.r = 0.0; // Skip vertical processing.
        }
#endif  // SMAA_USE_DIAG_DETECTION
    }

    SMAA_BRANCH
    if (e.r > 0.0)
    { // Edge at west
        float2 d;

        // Find the distance to the top:
        float3 coords;
        coords.y = SMAASearchYUp(SMAATexturePass2D(edgesTex), SMAATexturePass2D(searchTex), offset[1].xy, offset[2].z);
        coords.x = offset[0].x; // offset[1].x = texcoord.x - 0.25 * SMAA_RT_METRICS.x;
        d.x = coords.y;

        // Fetch the top crossing edges:
        float e1 = SMAASampleLevelZero(edgesTex, coords.xy).g;

        // Find the distance to the bottom:
        coords.z = SMAASearchYDown(SMAATexturePass2D(edgesTex), SMAATexturePass2D(searchTex), offset[1].zw, offset[2].w);
        d.y = coords.z;

        // We want the distances to be in pixel units:
        d = abs(round(mad(SMAA_RT_METRICS.ww, d, -pixcoord.yy)));

        // SMAAArea below needs a sqrt, as the areas texture is compressed
        // quadratically:
        float2 sqrt_d = sqrt(d);

        // Fetch the bottom crossing edges:
        float e2 = SMAASampleLevelZeroOffset(edgesTex, coords.xz, int2(0, API_V_DIR(1))).g;

        // Get the area for this direction:
        weights.ba = SMAAArea(SMAATexturePass2D(areaTex), sqrt_d, e1, e2, subsampleIndices.x);

        // Fix corners:
        coords.x = texcoord.x;
        SMAADetectVerticalCornerPattern(SMAATexturePass2D(edgesTex), weights.ba, coords.xyxz, d);
    }

    return weights;
}


void main() 
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

    vec4 weight = SMAABlendingWeightCalculationCSInno(pos, out_img, in_edgePoint, in_edge, in_area, in_search, subsampleIndices);

    imageStore(out_img, pos, weight);
}
