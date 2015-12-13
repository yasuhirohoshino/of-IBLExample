#version 400

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform vec4 globalColor;

uniform mat4 viewTranspose;

in vec4  position;
in vec4  color;
in vec3  normal;
in vec2  texcoord;

out vec4 colorVarying;
out vec2 texCoordVarying;

out vec3 normalVarying;
out vec4 positionVarying;

out vec3 v_normalVarying;
out vec4 v_positionVarying;

out vec3 reflectVec;
out mat3 mView;
out vec3 viewVec;

void main() {
    normalVarying = normal;
    positionVarying = position;
    
    vec3 norm = normalize(vec3(transpose(inverse(modelViewMatrix)) * vec4(normal, 1.0)));
    vec3 tang = normalize(cross(norm, vec3(1, 1, 1)));
    vec3 binormal = normalize(cross(norm, tang));
    
    mView = mat3(tang, binormal, norm);
    
    viewVec.x = dot(tang, -position.xyz);
    viewVec.y = dot(binormal, -position.xyz);
    viewVec.z = dot(norm, -position.xyz);
    
    viewVec = normalize(viewVec);
    
    v_normalVarying = normalize(vec3(transpose(inverse(modelViewMatrix)) * vec4(normal, 1.0)));
    v_positionVarying = modelViewMatrix * vec4(position.xyz, 1.0);
    
    texCoordVarying = texcoord;
    
    vec3 relfect0 = reflect(normalize(v_positionVarying.xyz), v_normalVarying);
    reflectVec = vec3(viewTranspose * vec4(relfect0, 0.0)) * vec3(1, 1, -1);
    
    colorVarying = globalColor;
    
    gl_Position = modelViewProjectionMatrix * position;
}