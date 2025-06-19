#version 450

layout(location = 0) out vec2 fragTexCoord;

void main() {
    // Generate a full-screen quad using gl_VertexIndex
    fragTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2); // 0,0 2,0 0,2 2,2 gives UVs
    gl_Position = vec4(fragTexCoord * 2.0 - 1.0, 0.0, 1.0); // -1 to 1 for clip space
}