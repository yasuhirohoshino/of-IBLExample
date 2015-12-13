#version 400

#define PI 3.14159265358979
#define TwoPI 6.28318530718

uniform samplerCube envMap;
uniform sampler2D tex;
uniform float roughness;
uniform float metallic;
uniform float cameraFar;
uniform mat4 viewTranspose;

uniform int useBaseColorTex;
uniform sampler2D baseColorTex;
uniform int useRoughnessTex;
uniform sampler2D roughnessTex;
uniform int useMetallicTex;
uniform sampler2D metallicTex;
uniform int useNormalTex;
uniform sampler2D normalMap;

uniform int isHDR;

in vec4 colorVarying;
in vec2 texCoordVarying;

in vec3 normalVarying;
in vec4 positionVarying;
in vec3 v_normalVarying;
in vec4 v_positionVarying;

in vec3 reflectVec;
in mat3 mView;
in vec3 viewVec;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 normalOut;

vec3 PrefilterEnvMap(float Roughness, vec3 R)
{
    vec4 color = mix(textureLod( envMap, R, int(Roughness * 9) ), textureLod( envMap, R, min(int(Roughness * 9) + 1, 9)), fract(Roughness * 9));
    return color.rgb;
}

// Env Specular
vec3 EnvBRDFApprox( vec3 SpecularColor, float Roughness, float NoV )
{
    vec4 c0 = vec4( -1, -0.0275, -0.572, 0.022 );
    vec4 c1 = vec4( 1, 0.0425, 1.04, -0.04 );
    vec4 r = Roughness * c0 + c1;
    float a004 = min( r.x * r.x, exp2( -9.28 * NoV ) ) * r.x + r.y;
    vec2 AB = vec2( -1.04, 1.04 ) * a004 + r.zw;
    return SpecularColor * AB.x + AB.y;
}

vec3 ApproximateSpecularIBL(vec3 SpecularColor, float Roughness, vec3 N, vec3 V, vec3 ReflectDir)
{
    float NoV = dot(N, V);
    vec3 PrefilteredColor = PrefilterEnvMap( Roughness, ReflectDir );
    return SpecularColor * PrefilteredColor * EnvBRDFApprox(SpecularColor, Roughness, NoV);
}

// Fresnel
vec3 Fresnel(vec3 N, vec3 V, float Roughness, vec3 ReflectDir, float f0){
    float base = 1.0 - clamp(dot(N, V), 0.0, 0.99);
    float exponential = pow(base, 5.0);
    float fresnel = f0 + (1.0 - f0) * exponential;
    vec3 reflectColor = PrefilterEnvMap(Roughness, ReflectDir);
    return reflectColor * fresnel;
}

void main (void)
{
    vec3 baseColor = vec3(1.0);
    float gamma = 1.0;
    if(isHDR == 1){
        gamma = 2.2;
    }
    if(useBaseColorTex != 1){
        baseColor = pow(colorVarying.xyz, vec3(gamma));
    } else {
        vec3 baseColorTexColor = texture(baseColorTex, mod(texCoordVarying * 1.0, 1.0)).rgb;
        baseColor = pow(baseColorTexColor * colorVarying.xyz, vec3(gamma));
    }
    
    float roughnessVal = 0.0;
    if(useRoughnessTex != 1){
        roughnessVal = roughness;
    } else {
        roughnessVal = texture(roughnessTex, mod(texCoordVarying * 1.0, 1.0)).r;
    }
    
    float metallicVal = 0.0;
    if(useMetallicTex != 1){
        metallicVal = metallic;
    } else {
        metallicVal = texture(metallicTex, mod(texCoordVarying * 1.0, 1.0)).r;
    }
    
    vec3 normal = vec3(0.0);
    vec3 reflectDir = vec3(0.0);
    vec3 viewDir = vec3(0.0);
    if(useNormalTex != 1){
        normal = v_normalVarying;
        reflectDir = reflectVec;
        viewDir = normalize(-v_positionVarying.xyz);
    } else {
        normal = mView * ((texture(normalMap, texCoordVarying) - 0.5) * 2.0).rgb;
        vec3 relfect0 = reflect(normalize(v_positionVarying.xyz), normal);
        reflectDir = vec3(viewTranspose * vec4(relfect0, 0.0)) * vec3(1, 1, -1);
        viewDir = viewVec;
    }
    
    vec3 diffuseColor = baseColor - baseColor * metallicVal;
    vec3 specularColor	= mix(vec3(0.01), baseColor, metallicVal);
    
    vec3 diffuse = textureLod(envMap, normal, 6).rgb * diffuseColor;
    vec3 specular = ApproximateSpecularIBL(specularColor, roughnessVal, normal, viewDir, reflectDir);
    vec3 fresnel = Fresnel(normal, viewDir, roughnessVal, reflectDir, 0.02) * (1.0 - metallicVal);
    
    fragColor = vec4(vec3(diffuse + specular + fresnel), 1.0);
    normalOut = vec4(normal, -v_positionVarying.z / cameraFar);
}