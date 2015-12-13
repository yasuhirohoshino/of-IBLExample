#version 150

uniform mat4 modelViewProjectionMatrix;

in vec4  position;
in vec4  color;
in vec3  normal;
in vec2  texcoord;

out vec2 texCoordVarying;

void main() {
	gl_Position = modelViewProjectionMatrix * position;
    texCoordVarying = texcoord;
}