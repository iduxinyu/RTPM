#version 430 core


#define MAT_DIFFUSE 0
#define MAT_SPECULAR 1
#define MAT_REFRACT 2

out vec4 FragColor;

in VS_OUT
{
    vec3 normal;
    vec2 texcoord;
    vec3 lightDir;
    vec4 color;
    vec2 screenPos;
} fs_in;

layout(binding = 1) uniform sampler2D DepthTexture;
layout(binding = 2) uniform sampler2D NormalTexture;
layout(binding = 3) uniform sampler2D AlbedoTexture;

uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

uniform float ZTolerance;
uniform float MinRoughness;



float SmoothKernel(float x)
{
    x = clamp(x, 0.0, 1.0);
    return x * x * (2.0 * x - 3.0) + 1.0;
}



// -----------------------------
// Utility
// -----------------------------
float saturate(float x) { return clamp(x, 0.0, 1.0); }

float Pow4(float x)
{
    float x2 = x * x;
    return x2 * x2;
}

// -----------------------------
// GGX Normal Distribution Function
// -----------------------------
float D_GGX(float a2, float NoH)
{
    float d = (NoH * NoH) * (a2 - 1.0) + 1.0;
    return a2 / (3.14159265 * d * d);
}

// -----------------------------
// Implicit visibility (very cheap)
// -----------------------------
float Vis_Implicit()
{
    return 0.25; // UE 常见近似（等价于 G/4）
}

// -----------------------------
// Fresnel (disabled version)
// -----------------------------
vec3 F_None(vec3 SpecularColor)
{
    return SpecularColor;
}

// -----------------------------
// Lambert diffuse
// -----------------------------
vec3 Diffuse_Lambert(vec3 DiffuseColor)
{
    return DiffuseColor / 3.14159265;
}

// -----------------------------
// Energy terms
// -----------------------------
struct EnergyTerms
{
    float diffuseFactor;
    float specularFactor;
};


EnergyTerms ComputeGGXSpecEnergyTerms(float roughness, float NoV, vec3 SpecularColor)
{
    EnergyTerms e;


    float specEnergy = max(max(SpecularColor.r, SpecularColor.g), SpecularColor.b);

    e.specularFactor = 1.0;
    e.diffuseFactor  = 1.0 - specEnergy;

    return e;
}

float ComputeEnergyPreservation(EnergyTerms e)
{
    return e.diffuseFactor;
}

float ComputeEnergyConservation(EnergyTerms e)
{
    return e.specularFactor;
}

// -----------------------------
// SimpleShading
// -----------------------------
vec3 SimpleShading(
    vec3 DiffuseColor,
    vec3 SpecularColor,
    float Roughness,
    vec3 L,
    vec3 V,
    vec3 N)
{
    float NoV = saturate(dot(N, V));

    EnergyTerms energy = ComputeGGXSpecEnergyTerms(Roughness, NoV, SpecularColor);

    vec3 H = normalize(V + L);
    float NoH = saturate(dot(N, H));

    float a2 = Pow4(Roughness);

    // Specular
    float D = D_GGX(a2, NoH);
    float Vis = Vis_Implicit();
    vec3  F = F_None(SpecularColor);

    vec3 specular = (D * Vis) * F * ComputeEnergyConservation(energy);

    // Diffuse
    vec3 diffuse = Diffuse_Lambert(DiffuseColor) * ComputeEnergyPreservation(energy);

    return diffuse + specular;
}

vec3 GetSimplePhotonLighting(vec3 CameraVector, vec3 WorldNormal, vec3 PhotonDirection, vec3 DiffuseColor, vec3 SpecularColor, float Roughness)
{
	vec3 V = -CameraVector;
	vec3 N = WorldNormal;
	vec3 L = PhotonDirection;

	vec3 OutLighting = SimpleShading(DiffuseColor, SpecularColor, max(Roughness, .04f), L, V, N);  //get light BRDF for photon

	return OutLighting;
}

void DecodeGBuffer(vec2 uv, inout vec3 DiffuseColor, inout vec3 SpecularColor, inout vec3 Normal, inout float SceneDepth, inout float Roughness)
{
    Normal=texture(NormalTexture,uv).xyz;

    DiffuseColor=texture(AlbedoTexture,uv).xyz;

    SceneDepth=texture(DepthTexture, uv).x;

    float matType=texture(NormalTexture,uv).w;
    if(int(matType)==MAT_DIFFUSE)
    {
        Roughness=1.0;
        SpecularColor=vec3(0.01);
    }
    else if(int(matType)==MAT_SPECULAR)
    {
        Roughness=0.01;
        SpecularColor=vec3(0.9);
    }
    else if(int(matType)==MAT_REFRACT)
    {
        Roughness=0.01;
        SpecularColor=vec3(0.04); //glass specular color
    }


}

void main()
{
   
    float gKernelPower = 1;
    //get material property by GBuffer
    vec3 DiffuseColor;
    vec3 SpecularColor;
    vec3 Normal;
    float Roughness;
    float SceneDepth;

    DecodeGBuffer(fs_in.screenPos, DiffuseColor, SpecularColor, Normal, SceneDepth, Roughness);

    //Scene View pos
    vec4 sceneViewPos= inverse(ProjectionMatrix)*vec4(fs_in.screenPos*2.0-1.0,SceneDepth*2.0-1.0,1.0);
    sceneViewPos /= sceneViewPos.w;
    float sceneViewDepth = sceneViewPos.z;

    //photon View pos

    float photonViewDepth=-1.0/gl_FragCoord.w;

    float zDiff = (photonViewDepth - sceneViewDepth) / ZTolerance;

    float normalDiff = dot(Normal, normalize(fs_in.normal));

    if (abs(zDiff) > 1 || normalDiff < 0)
	{
		discard;
	}


    //result color = photonColor * object surface Color
    vec3 LightingColor = fs_in.color.rgb *GetSimplePhotonLighting(-sceneViewPos.xyz, Normal, fs_in.lightDir, DiffuseColor, SpecularColor, Roughness);

    float alpha;
	
	alpha = SmoothKernel(length(vec3(fs_in.texcoord.xy, zDiff)));
	alpha = pow(alpha, gKernelPower) * pow(normalDiff, 16);
	
    //foot print
    const float screenPhotonRadius = 0.1;
	float photonScreenArea = fs_in.color.a;
	float photonFootprint = dot(fs_in.texcoord.xy, fs_in.texcoord.xy) * photonScreenArea < screenPhotonRadius * screenPhotonRadius ? 1 : 0;

    FragColor = vec4(LightingColor*alpha, photonFootprint);
    //FragColor = vec4(LightingColor, 1.0);

    //FragColor = vec4(abs(sceneViewDepth), 0.0,0.0,1.0);

    //FragColor = vec4(1.0,0.0,0.0,1.0);
}