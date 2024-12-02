#define PI 3.14159265359
#define GOLD vec3(1.0, 0.765557, 0.336057)

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool projectSphere(vec3 c, float r, float znear, float P00, float P11, out vec4 aabb)
{
    if (c.z < r + znear) // (znear > c.z - r)
        return false;

    vec3 cr = c * r;
    float czr2 = c.z * c.z - r * r;

    float vx = sqrt(c.x * c.x + czr2);
    float minx = (vx * c.x - cr.z) / (vx * c.z + cr.x);
    float maxx = (vx * c.x + cr.z) / (vx * c.z - cr.x);

    float vy = sqrt(c.y * c.y + czr2);
    float miny = (vy * c.y - cr.z) / (vy * c.z + cr.y);
    float maxy = (vy * c.y + cr.z) / (vy * c.z - cr.y);

    aabb = vec4(minx * P00, miny * P11, maxx * P00, maxy * P11);
    aabb = aabb.xwzy * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f); // clip space -> uv space

    return true;
}


bool coneCull(vec3 center, float radius, vec3 cone_axis, float cone_cutoff, vec3 camera_position)
{
    return dot(center - camera_position, cone_axis) >= (cone_cutoff * length(center - camera_position) + radius);
}

vec3 rotateQuat(vec3 v, vec4 q)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

float boundsError(LodBounds bounds, vec3 cam_pos, float cam_proj_1_1, float cam_znear)
{
    float dx = bounds.center.x - cam_pos.x;
    float dy = bounds.center.y - cam_pos.y;
    float dz = bounds.center.z - cam_pos.z;
    float d = sqrt(dx * dx + dy * dy + dz * dz) - bounds.radius;

    float limd = max(d, cam_znear);
    return bounds.error / (limd * cam_proj_1_1 * 0.5);
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = dotNH * dotNH * (a2 - 1.0) + 1.0;
    return (a2) / (PI * denom * denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotnl, float dotnv, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float GL = dotnl / (dotnl * (1.0 - k) + k);
    float GV = dotnv / (dotnv * (1.0 - k) + k);
    return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float dotnv, float metallic, vec3 baseColor)
{
    // F0 for dielectrics, can be understand as the reflectance at 0 degree.
    // it is more like a exponential function, the metallic material has a higher reflectance at 0 degree.
    vec3 F0 = mix(vec3(0.04), baseColor, smoothstep(0., 1., metallic));
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - dotnv, 5.0);
    return F;
}

float F_Schlick(float f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5);
}

// Burley diffuse BRDF --------------------------------------------------
float Fd_Burley(float roughness, float dotnv, float dotnl, float dotlh) {
    float linearRoughness = roughness * roughness;
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * linearRoughness * dotlh * dotlh;
    float lightScatter = F_Schlick(1.0, f90, dotnl);
    float viewScatter = F_Schlick(1.0, f90, dotnv);
    return lightScatter * viewScatter * (1.0 / PI);
}

// Specular BRDF composition --------------------------------------------

vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 C, float metallic, float roughness)
{
    // Precalculate vectors and dot products    
    vec3 H = normalize(V + L);
    float dotnv = clamp(dot(N, V), 0.0, 1.0);
    float dotnl = clamp(dot(N, L), 0.0, 1.0);
    float dotlh = clamp(dot(L, H), 0.0, 1.0);
    float dotnh = clamp(dot(N, H), 0.0, 1.0);

    vec3 color = vec3(0.0);

    if (dotnl > 0.0)
    {
        float rroughness = max(0.05, roughness);
        // D = Normal distribution (Distribution of the microfacets)
        float D = D_GGX(dotnh, roughness);
        // G = Geometric shadowing term (Microfacets shadowing)
        float G = G_SchlicksmithGGX(dotnl, dotnv, rroughness);
        // F = Fresnel factor (Reflectance depending on angle of incidence)
        vec3 F = F_Schlick(dotnl, metallic, C);

        vec3 spec = D * F * G / (4.0 * dotnl * dotnv);

        color = spec;
    }

    return color;
}