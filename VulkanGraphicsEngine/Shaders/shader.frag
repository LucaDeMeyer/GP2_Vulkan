#version 450

// Vertex shader outputs (inputs to this fragment shader)
layout(location = 0) in vec3 fragColor; 
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 WorldPos;
layout(location = 4) in vec3 fragTangent;

// Output attachments for the G-Buffer
layout(location = 0) out vec4 gAlbedo;          
layout(location = 1) out vec4 gAO;              
layout(location = 2) out vec4 gNormal;          
layout(location = 3) out vec4 gMetallicRoughness;
layout(location = 4) out vec4 gWorldPos;

// Texture maps (inputs for material properties)
layout(binding = 2) uniform sampler2D albedoMap;
layout(binding = 3) uniform sampler2D normalMap;
layout(binding = 4) uniform sampler2D metallicMap;
layout(binding = 5) uniform sampler2D roughnessMap;
layout(binding = 6) uniform sampler2D aoMap;

layout(push_constant) uniform Push {
    mat4 transform;    
    mat4 modelMatrix; 
} push;

layout(binding = 1) uniform ModelUniformBufferObject { // Matches globalBinding[1]
    mat4 model; 
} modelUBO;


void main() {
     // Sample material properties from textures
    vec3 albedoColor = texture(albedoMap, fragTexCoord).rgb;
    vec3 tangentNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;  
    float metallic = texture(metallicMap, fragTexCoord).r;
    float roughness = texture(roughnessMap, fragTexCoord).r;
    float ao = texture(aoMap, fragTexCoord).r;


    vec3 T = normalize(fragTangent);
    vec3 N = normalize(fragNormal);
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    vec3 worldSpaceNormal = normalize(TBN * tangentNormal); 

    // Output to G-Buffer attachments
    gAlbedo = vec4(albedoColor, 1.0); 
    gAO = vec4(ao, 0.0, 0.0, 1.0);   
    gNormal = vec4(worldSpaceNormal * 0.5 + 0.5, 1.0);
    gMetallicRoughness = vec4(metallic, roughness, 0.0, 1.0);
    gWorldPos = vec4(WorldPos, 1.0);    
}
