#version 450

// Input fragment coordinates from the fullscreen quad (usually 0 to 1)
layout(location = 0) in vec2 fragTexCoord; // Fullscreen quad UVs

// G-Buffer texture inputs
layout(binding = 0) uniform sampler2D gAlbedo;
layout(binding = 1) uniform sampler2D gAO;
layout(binding = 2) uniform sampler2D gNormal;
layout(binding = 3) uniform sampler2D gMetallicRoughness;
layout(binding = 4) uniform sampler2D gDepth; // We'll sample depth to reconstruct world position

// Output for the HDR color image
layout(location = 0) out vec4 outColor;

// Camera and Light Uniforms (same as before)
layout(binding = 9) uniform CameraUniformBufferObject { // Binding 9 for CameraUBO
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} cameraUBO;

struct PointLight{
    vec3 position;
    vec3 color;
    float lumen;
    float radius;
};
struct DirectionalLight{
    vec3 direction;
    vec3 color;
    float lux;
};

layout(binding = 8 ) uniform LightUniformBufferObject
{
    PointLight Pointlights[4];
    DirectionalLight DirectionalLight;
    int numlights;
}LightUBO;

// Push constants for screen size (to reconstruct world position from depth)
layout(push_constant) uniform ScreenSizePush {
    vec2 inverseScreenSize; // 1.0 / width, 1.0 / height
    mat4 inverseViewProjection; // Inverse of cameraUBO.proj * cameraUBO.view
} screenSizePush;

const float PI = 3.14159265359;

// FRESNELSHLICK
vec3 fresnelShlick(float costTheta, vec3 F0)
{
    return F0 + (vec3(1.0) - F0) * pow(clamp(1.0 - costTheta, 0.0, 1.0), 5.0);
}

// DISTRIBUTION GGX
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N,H),0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

// GEOMETRIC SCHLICKGGX
float GeometricSchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r*r)/8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

// GEOMETRIC SMITH
float geometricSmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N,V),0.0);
    float NdotL = max(dot(N,L),0.0);
    float ggx2 = GeometricSchlickGGX(NdotV, roughness);
    float ggx1 = GeometricSchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Function to reconstruct world position from depth buffer and camera parameters
vec3 reconstructWorldPosition(vec2 uv, float depth, mat4 invVP) {
    // Convert UV to normalized device coordinates (NDC)
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth, 1.0);
    // Transform from clip space to world space
    vec4 worldPos = invVP * clipSpace;
    return worldPos.xyz / worldPos.w; // Perspective divide
}

void main() {
    // Sample G-Buffer textures
    vec3 albedoColor = texture(gAlbedo, fragTexCoord).rgb;
    float ao = texture(gAO, fragTexCoord).r; // AO is in the red channel

    vec3 N = normalize(texture(gNormal, fragTexCoord).rgb);

    vec4 metallicRoughness = texture(gMetallicRoughness, fragTexCoord);
    float metallic = metallicRoughness.r;
    float roughness = metallicRoughness.g;

    // Reconstruct world position from depth
    float depth = texture(gDepth, fragTexCoord).r; // Depth is usually in R channel
    vec3 WorldPos = reconstructWorldPosition(fragTexCoord, depth, screenSizePush.inverseViewProjection);

    vec3 V = normalize(cameraUBO.cameraPos - WorldPos);

    vec3 Lo = vec3(0.0);

    // Point Lights
    for(int i = 0; i < LightUBO.numlights; ++ i)
    {
        vec3 L = normalize(LightUBO.Pointlights[i].position - WorldPos);
        vec3 H = normalize(V + L);

        float distance = length(LightUBO.Pointlights[i].position - WorldPos);

        float normalizedDistance = clamp(distance / LightUBO.Pointlights[i].radius, 0.0, 1.0);
        float attenuationFactor = 1.0;
        if (distance > 0.0) {
            float invSqAttenuation = 1.0 / (distance * distance);
            float falloffCurve = 1.0 - pow(normalizedDistance, 2.0);
            falloffCurve = max(0.0, falloffCurve);
            attenuationFactor = invSqAttenuation * falloffCurve;
        } else {
            attenuationFactor = 1000.0;
        }

        vec3 radiance = LightUBO.Pointlights[i].color * LightUBO.Pointlights[i].lumen * attenuationFactor;

        vec3 F0_dielectric = vec3(0.04);
        vec3 F0 = mix(F0_dielectric, albedoColor, metallic);
        vec3 F = fresnelShlick(max(dot(H, V), 0.0), F0);

        float NDF = distributionGGX(N, H, roughness);
        float G = geometricSmith(N, V, L, roughness);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedoColor / PI + specular) * radiance * NdotL;
    }

    // Directional Light
    vec3 L_dir = normalize(-LightUBO.DirectionalLight.direction); // Renamed to avoid conflict
    vec3 H_dir = normalize(V + L_dir); // Renamed to avoid conflict
    vec3 radiance_dir = LightUBO.DirectionalLight.color * LightUBO.DirectionalLight.lux;

    vec3 F0_dielectric_dir = vec3(0.04);
    vec3 F0_dir = mix(F0_dielectric_dir, albedoColor, metallic);
    vec3 F_dir = fresnelShlick(max(dot(H_dir, V), 0.0), F0_dir);

    float NDF_dir = distributionGGX(N, H_dir, roughness);
    float G_dir = geometricSmith(N, V, L_dir, roughness);

    vec3 numerator_dir = NDF_dir * G_dir * F_dir;
    float denominator_dir = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L_dir), 0.0) + 0.001;
    vec3 specular_dir = numerator_dir / denominator_dir;

    vec3 kS_dir = F_dir;
    vec3 kD_dir = vec3(1.0) - kS_dir;
    kD_dir *= 1.0 - metallic;

    float NdotL_dir = max(dot(N, L_dir), 0.0);
    Lo += (kD_dir * albedoColor / PI + specular_dir) * radiance_dir * NdotL_dir;

    // Ambient term
    vec3 ambient = vec3(0.03) * albedoColor * ao; // Use the AO from G-Buffer

    vec3 color = ambient + Lo;

    outColor = vec4(color, 1.0);
}
