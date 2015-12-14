#version 150

#define PI 3.14159265358979

uniform sampler2D tex;
uniform samplerCube envMap;
uniform float Roughness;

in vec4 colorVarying;
in vec2 texCoordVarying;
in vec3 normalVarying;
in vec4 positionVarying;

out vec4 fragColor;

float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(
                float(i) / float(N),
                radicalInverse_VdC(i)
                );
}

vec3 ImportanceSampleGGX( vec2 Xi, float Roughness, vec3 N ) {
    float a = Roughness * Roughness;
    
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a*a - 1) * Xi.y ) );
    float SinTheta = sqrt( 1 - CosTheta * CosTheta );
    
    vec3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;
    
    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 TangentX = normalize( cross( UpVector, N ) );
    vec3 TangentY = cross( N, TangentX );
    // Tangent to world space
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

float G_Smith(float Roughness, float NoV, float NoL)
{
    float a = Roughness * Roughness;
    float k = a / 2.0;
    
    //Source: UE4
    float G1l = NoL / (NoL * (1.0 - k) + k);
    float G1v = NoV / (NoV * (1.0 - k) + k);
    
    float Glvn = G1l * G1v;
    return Glvn;
}

vec3 prefilterEnvMap(float Roughness, vec3 R){
    vec3 N = R;
    vec3 V = R;
    vec3 PrefilteredColor = vec3(0.0);
    
    const int NumSamples = 1024;
    float totalsample = 0.0;
    
    for(int i=0; i<NumSamples; i++) {
        vec2 Xi = Hammersley(uint(i), uint(NumSamples));
        vec3 H = ImportanceSampleGGX(Xi, Roughness, N);
        vec3 L = 2.0 * dot(V, H) * H - V;
        
        float NoL = clamp(dot(N, L), 0, 1);
        
        if (NoL > 0.0) {
            vec3 SampleColor = texture( envMap, L, 0 ).rgb;
            PrefilteredColor += SampleColor;
            totalsample++;
        }
    }
    return PrefilteredColor / totalsample;
}

void main (void) {
    fragColor = vec4(prefilterEnvMap(Roughness, normalVarying),1.0);
}
