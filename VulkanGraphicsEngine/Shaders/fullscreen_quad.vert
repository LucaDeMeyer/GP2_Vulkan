#version 450

layout(location = 0) out vec2 fragTexCoord;

void main() {
    // Generate a fullscreen quad using gl_VertexIndex
    // This maps vertex index to clip space coordinates and UVs
    // Vertex 0: (0,0) -> (-1,-1)
    // Vertex 1: (1,0) -> ( 1,-1)
    // Vertex 2: (0,1) -> (-1, 1)
    // Vertex 3: (1,1) -> ( 1, 1) (if using 2 triangles)
    // For 3 vertices (triangle strip or single triangle covering screen):
    // gl_VertexIndex 0: (-1, -1), UV (0, 0)
    // gl_VertexIndex 1: ( 3, -1), UV (2, 0)
    // gl_VertexIndex 2: (-1,  3), UV (0, 2)
    // This creates a triangle that covers the entire screen.
    fragTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(fragTexCoord * 2.0 - 1.0, 0.0, 1.0);
}
