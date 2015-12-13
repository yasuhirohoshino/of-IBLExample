#version 150

uniform mat4 orientationMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;
uniform mat4 modelViewProjectionMatrix;

in vec4  position;
in vec4  color;
in vec3  normal;
in vec2  texcoord;

out vec4 colorVarying;
out vec2 texCoordVarying;

out vec3 normalVarying;
out vec4 positionVarying;

void main() {
	gl_Position = modelViewProjectionMatrix * position;
    colorVarying = color;
    texCoordVarying = texcoord;
    
    normalVarying = normal;
    positionVarying = position;
}