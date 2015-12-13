#version 150

#define PI 3.14159265358979

uniform sampler2D tex;
in vec2 texCoordVarying;

out vec4 fragColor;

void main (void) {
    fragColor = texture(tex, texCoordVarying).bgra;
}
