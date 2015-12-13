#version 400

#define PI 3.14159265358979
#define TwoPI 6.28318530718

out vec4 fragColor;

in vec2 texCoordVarying;

uniform sampler2D base;
uniform sampler2D ssao;
uniform vec2 resolution;
uniform float gamma;
uniform float exposure;

// Linear
vec3 toLinear(vec3 v) {
    return pow(v, vec3(gamma));
}

vec4 toLinear(vec4 v) {
    return vec4(toLinear(v.rgb), v.a);
}

// gamma
vec3 toGamma(vec3 v) {
    return pow(v, vec3(1.0 / gamma));
}

vec4 toGamma(vec4 v) {
    return vec4(toGamma(v.rgb), v.a);
}

// tonemap
vec3 tonemapReinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

void main (void) {
    vec3 color = texture(base, texCoordVarying).rgb;
    float ssaoVal = texture(ssao, texCoordVarying).r;
    color *= ssaoVal;
    color *= exposure;
    color = tonemapReinhard(color);
    color = toGamma(color);

    fragColor = vec4(color, 1.0);
}