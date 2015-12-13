#version 400

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseProjectionMatrix;

in vec4  position;
in vec4  color;
in vec3  normal;
in vec2  texcoord;

out vec2 texCoordVarying;

out mat4 invProj;

void main() {
    texCoordVarying = texcoord;
    invProj = inverseProjectionMatrix;
    gl_Position = modelViewProjectionMatrix * position;
}