#version 450

// Camera Uniform Buffer Object
layout(binding = 0) uniform CameraUniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} cameraUBO;

layout(binding = 1) uniform ModelUniformBufferObject { // Matches globalBinding[1]
    mat4 model; 
} modelUBO;

// Input vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor; // Not used in GBuffer, but kept for consistency
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

// Output to fragment shader (GBuffer inputs)
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;   // World-space normal
layout(location = 3) out vec3 WorldPos;     // World-space position
layout(location = 4) out vec3 fragTangent;  // World-space tangent

// Push constants
layout(push_constant) uniform Push {
    mat4 transform;     // Typically MVP matrix
    mat4 modelMatrix;   // Model matrix
} push;

void main() {
    // Calculate world-space position
    vec4 positionWorld = modelUBO.model  * vec4(inPosition, 1.0);
    WorldPos = positionWorld.xyz;

    // Calculate clip-space position
    gl_Position = cameraUBO.proj * cameraUBO.view * positionWorld;

    // Pass texture coordinates directly
    fragTexCoord = inTexCoord;

    // Calculate normal matrix for correct normal transformation
    // Use inverse transpose of the model matrix for normals and tangents
    // This handles non-uniform scaling correctly.
    mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));

    // Transform normals and tangents to world space
    fragNormal = normalize(normalMatrix * inNormal);
    fragTangent = normalize(normalMatrix * inTangent);

    // Pass color (if needed, otherwise can be removed)
    fragColor = inColor;
}
