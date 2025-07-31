// ----------------------------------------------------------------
// most of the pbr material functions are grab from:
// https://www.shadertoy.com/view/XlKSDR
// and filament engine document:
// https://google.github.io/filament/Filament.html
// Note: filament seems assume light and view direction are from source to object
//       but in this case, we assume light and view direction are from object to source
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

float D_GGX(float linearRoughness, float NoH) {
    float a2 = linearRoughness;
    float k = (1.0 + (NoH * NoH) * (a2 - 1.0));
    float d = (NoH * a2) / (PI * k * k);
    return d;
}

float V_SmithGGXCorrelated(float linearRoughness, float NoV, float NoL) {
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    float a2 = linearRoughness;// *linearRoughness;
    float GGXV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float GGXL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    return 0.5 / (GGXV + GGXL);
}

// assumming f90 = 1
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


//------------------------------------------------------------------------------
// Indirect lighting
//------------------------------------------------------------------------------

vec3 Irradiance_SphericalHarmonics(const vec3 n) {
    // Irradiance from "Ditch River" IBL (http://www.hdrlabs.com/sibl/archive.html)
    return max(
        vec3(0.754554516862612, 0.748542953903366, 0.790921515418539)
        + vec3(-0.083856548007422, 0.092533500963210, 0.322764661032516) * (n.y)
        + vec3(0.308152705331738, 0.366796330467391, 0.466698181299906) * (n.z)
        + vec3(-0.188884931542396, -0.277402551592231, -0.377844212327557) * (n.x)
        , 0.0);
}

vec2 PrefilteredDFG_Karis(float roughness, float NoV) {
    // Karis 2014, "Physically Based Material on Mobile"
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.040, -0.040);

    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;

    return vec2(-1.04, 1.04) * a004 + r.zw;
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
