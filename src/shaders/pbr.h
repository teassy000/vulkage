// ----------------------------------------------------------------
// most of the pbr material functions are grab from:
// https://www.shadertoy.com/view/XlKSDR
// ----------------------------------------------------------------
// following symbol correspond to...
// 
//  v:      view direction unit vector, from surface to camera
//  l:      incident light direction unit vector, from surface to light
//  n:      surface normal unit vector
//  a:      linear roughness, which is (roughness * roughness)
//  F:      Fresnel
//  D:      normal distribution
//  V:      geometric shadowing, with V == G_SmithGGX /  (4 * NoV * NoL)
//  Fd:     diffuse component of BRDF
//  Fr:     specular component of BRDF
//  f0:     reflectance at normal incidence
//  f90:    reflectance at grazing angle

//  [N]o[M]: dot product between N and M
//  [N]x[M]: cross product between N and M


#define PI 3.14159265359
#define saturate(x) clamp(x, 0.0, 1.0)
//------------------------------------------------------------------------------
// BRDF
//------------------------------------------------------------------------------

float pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

float D_GGX(float linearRoughness, float NoH, const vec3 h) {
    // Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"
    float oneMinusNoHSquared = 1.0 - NoH * NoH;
    float a = NoH * linearRoughness;
    float k = linearRoughness / (oneMinusNoHSquared + a * a);
    float d = k * k * (1.0 / PI);
    return d;
}

float V_SmithGGXCorrelated(float linearRoughness, float NoV, float NoL) {
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    float a2 = linearRoughness * linearRoughness;
    float GGXV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float GGXL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    return 0.5 / (GGXV + GGXL);
}

vec3 F_Schlick(const vec3 f0, float VoH) {
    // Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
    return f0 + (vec3(1.0) - f0) * pow5(1.0 - VoH);
}

float F_Schlick(float f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

float Fd_Burley(float linearRoughness, float NoV, float NoL, float LoH) {
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * linearRoughness * LoH * LoH;
    float lightScatter = F_Schlick(1.0, f90, NoL);
    float viewScatter = F_Schlick(1.0, f90, NoV);
    return lightScatter * viewScatter * (1.0 / PI);
}

float Fd_Lambert() {
    return 1.0 / PI;
}

vec3 BRDF(vec3 v, vec3 n, vec3 l, vec3 c, float metal, float rough)
{
    vec3 h = normalize(v + l);
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = saturate(dot(n, l));
    float NoH = saturate(dot(n, h));
    float LoH = saturate(dot(l, h));
    
    vec3 f0 = mix(vec3(0.04), c, metal);
    vec3 diffuseColor = vec3(1.0) - f0;

    float linearRoughness = rough * rough;
    vec3 F = F_Schlick(f0, LoH);
    float D = D_GGX(linearRoughness, NoH, h);
    float V = V_SmithGGXCorrelated(linearRoughness, NoV, NoL);
    return F * D * V;
}


//------------------------------------------------------------------------------
// Tone mapping and transfer functions
//------------------------------------------------------------------------------
vec3 Tonemap_ACES(const vec3 x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

vec3 OECF_sRGBFast(const vec3 linear) {
    return pow(linear, vec3(1.0 / 2.2));
}
