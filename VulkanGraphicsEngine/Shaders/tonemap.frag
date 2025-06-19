#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D hdrTexture;

layout(push_constant) uniform ToneMapPush {
    float exposure;
    int tonemapOperator; // 0=Reinhard, 1=ACES, 2=Uncharted2
} push;

// Reinhard tone mapping
vec3 reinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

// ACES tone mapping
vec3 aces(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

// Uncharted 2 tone mapping
vec3 uncharted2Tonemap(vec3 color) {
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

vec3 uncharted2(vec3 color) {
    const float exposureBias = 2.0;
    vec3 curr = uncharted2Tonemap(exposureBias * color);
    vec3 whiteScale = 1.0 / uncharted2Tonemap(vec3(11.2));
    return curr * whiteScale;
}

void main() {
    vec3 hdrColor = texture(hdrTexture, fragTexCoord).rgb;
    
    // Apply exposure
    hdrColor *= push.exposure;
    
    // Apply tone mapping
    vec3 mapped;
    if (push.tonemapOperator == 0) {
        mapped = reinhard(hdrColor);
    } else if (push.tonemapOperator == 1) {
        mapped = aces(hdrColor);
    } else {
        mapped = uncharted2(hdrColor);
    }
    
    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));
    
    outColor = vec4(mapped, 1.0);
}