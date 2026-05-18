#version 430


layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Light types
const int LIGHT_POINT        = 0;
const int LIGHT_SPOT         = 1;
const int LIGHT_DIRECTIONAL  = 2;
const int LIGHT_RECT         = 3;

// Reflection / refraction control
const int CAUSTICS_FLAG_RANDOM_REFLECTION = 0;
const int CAUSTICS_FLAG_FORCE_REFLECTION  = 1;
const int CAUSTICS_FLAG_FORCE_REFRACTION  = 2;

// Material types
const int MAT_DIFFUSE  = 0;
const int MAT_SPECULAR = 1;
const int MAT_REFRACT  = 2;

const int AXIS_X=3;
const int AXIS_Y=4;
const int AXIS_Z=5;


const int MaxDepth= 4;   //bounds time

struct RayDesc
{
    vec3 o;
    vec3 d;
    float TMin;
    float TMax;
};

struct HitRecord{
	float t;
    vec3 color;
	vec3 normal;
	float matType;

    vec3 dPdu;
    vec3 dPdv;
    vec3 dNdu;
    vec3 dNdv;
	
};

struct CausticsUnpackedPayload
{
    vec3 color;
    vec3 nextDir;

    vec3 dDdx;
    vec3 dDdy;

    vec3 dPdx;
    vec3 dPdy;

    int continueFlag;
    float hitT;

};

struct Plane
{
    int axis;
    float distanceFromO;

    vec3 color;
    int matType;
    
};

struct Light
{
    //base property
    vec4 position;     
    // xyz = position
    // w   = radius

    vec4 direction;    
    // xyz = direction
    // w   = type

    vec4 color;        
    // rgb = intensity
    // w   = power

    vec4 spotPro;      
    // x = cosInner
    
    vec4 rectPro;
    // xyz Tangent

};

struct Photon
{
    vec4 posW;      // xyz + padding
    vec4 color;
    vec4 dPdx;
    vec4 dPdy;
    vec4 dir;
};

// Pixel info buffer
struct PixelInfo
{
    int count;
    int screenArea;
    int screenAreaSq;
};


layout(binding = 0) uniform sampler2D RayDensityTexture;

layout(std430, binding = 1) buffer RayCountBuffer
{
    uvec4 RayCountQuadTree[];
};

layout(std430, binding = 2) buffer MipOffsets
{
    uint mipOffset[];
};


layout(std430, binding =3) buffer LightBuffer
{
    Light lights[];
};

layout(std430, binding = 4) buffer PhotonBufferSSBO
{
    Photon photons[];
};

layout(std430, binding = 5) buffer PixelInfoBufferSSBO
{
    PixelInfo pixelInfo[];
};

// atomic counter for storing photons
layout(binding = 6, offset = 0) uniform atomic_uint photonCounter;

layout(binding = 7) uniform sampler2D VarianceTexture;
layout(binding = 8) uniform sampler2D SceneDepthBuffer;
layout(binding = 9) uniform sampler2D verticesTex;
                                        

uniform ivec2 LightMapSize;
uniform ivec2 ViewportDim;

uniform int mipDepth;

uniform float Intensity;
uniform float SplatSize;
//uniform int SoftSampleDim;
//uniform int EnableRandomReflection;
uniform float MidCullColorThreshold;
uniform float MaxBounceDistance;
uniform float MaxScreenRadius;

uniform vec2 verticesTexSize;

uniform mat4 ViewProjectionMatrix;

uniform int objNum;


/// local variable  //////////////////////////

//box
Plane planes[5];






//////////////////////////////////////////////
/// tools
/////////////////////////////////////////////



// PCG hash（高质量）
uint PCGHash(uint seed)
{
    uint state = seed * 747796405u + 2891336453u;
    uint word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Random(seed)
float Random(inout uint seed)
{
    seed = PCGHash(seed);
    return float(seed) * (1.0 / 4294967296.0); // [0,1)
}




////////////////////////////////////////////////////////
/// Get Light Info
////////////////////////////////////////////////////////

vec3 GetColor(uint LightID)
{
    return lights[LightID].color.xyz;
}
vec3 GetNormal(uint LightID)
{
    return lights[LightID].direction.xyz;
}
vec3 GetPosition(uint LightID)
{
    return lights[LightID].position.xyz;
}

float GetAttenuation(uint LightID)
{
    return lights[LightID].position.w;
}

int GetLightType(uint LightID)
{
    return int(lights[LightID].direction.w);
}

float GetWidth(uint LightID)
{
    return float(LightMapSize.y);
}

float GetHeight(uint LightID)
{
    return float(LightMapSize.x);
}

float GetCosConeAngle(uint LightID)
{
    return lights[LightID].spotPro.x;
}


vec3 GetdPdu(int lightId)
{
    return cross(lights[lightId].direction.xyz,
                 lights[lightId].rectPro.xyz);
}

vec3 GetdPdv(int lightId)
{
    return lights[lightId].rectPro.xyz;
}

mat3 GetTangentBasis(vec3 TangentZ)
{
    float sign_ = TangentZ.z >= 0.0 ? 1.0 : -1.0;
    float a = -1.0 / (sign_ + TangentZ.z);
    float b = TangentZ.x * TangentZ.y * a;

    vec3 TangentX = vec3(
        1.0 + sign_ * a * TangentZ.x * TangentZ.x,
        sign_ * b,
        -sign_ * TangentZ.x
    );

    vec3 TangentY = vec3(
        b,
        sign_ + a * TangentZ.y * TangentZ.y,
        -TangentZ.y
    );

    // column-major：X,Y,Z 是列
    return mat3(TangentX, TangentY, TangentZ);
}




float SpotAttenuationMask(vec3 L, vec3 SpotDirection, vec2 SpotAngles)
{
    float mask = (dot(L, -SpotDirection) - SpotAngles.x) * SpotAngles.y;
    return clamp(mask, 0.0, 1.0);
}

float SpotAttenuation(vec3 L, vec3 SpotDirection, vec2 SpotAngles)
{
    float coneAngleFalloff = SpotAttenuationMask(L, SpotDirection, SpotAngles);
    return coneAngleFalloff * coneAngleFalloff;
}




///////////////////////////////////////////////////////
/// For Mip Map
///////////////////////////////////////////////////////


uint GetTextureOffset(uvec2 threadIdx, int MipLevel)
{
    uint mipW = 1 << MipLevel; //左移  miplevel=0: mipW=1 ;   mipLevel=1: mipW=2;

    return mipOffset[MipLevel] + threadIdx.y * mipW + threadIdx.x;

}


bool GetSamplePos(uint threadId,
                  out uvec2 pixelPos,
                  out uint sampleIdx)
{
    pixelPos = uvec2(0);
    sampleIdx = threadId;

    uvec4 value = RayCountQuadTree[0];

    if(threadId >= value.w)
        return false;

    for(int mip=1; mip<=mipDepth; mip++)
    {
        pixelPos <<= 1u; //pixelPos *=2

        if(sampleIdx >= value.b)
        {
            pixelPos += uvec2(1,1);
            sampleIdx -= value.b;
        }
        else if(sampleIdx >= value.g)
        {
            pixelPos += uvec2(0,1);
            sampleIdx -= value.g;
        }
        else if(sampleIdx >= value.r)
        {
            pixelPos += uvec2(1,0);
            sampleIdx -= value.r;
        }

        uint nodeOffset = GetTextureOffset(pixelPos,mip);

        value = RayCountQuadTree[nodeOffset];
    }

    return true;
}

void GetRaySample(
    uvec2 pixel00,
    uint sampleIdx,
    out vec2 screenCoord,
    out vec2 pixelSize)
{
    pixel00*=2;
    float v00 =
      texelFetch(RayDensityTexture,
                 ivec2(pixel00),0).r;

    float v10 =
      texelFetch(RayDensityTexture,
                 ivec2(pixel00)+ivec2(1,0),0).r;

    float v01 =
      texelFetch(RayDensityTexture,
                 ivec2(pixel00)+ivec2(0,1),0).r;

    float v11 =
      texelFetch(RayDensityTexture,
                 ivec2(pixel00)+ivec2(1,1),0).r;

    float sampleCountF =
        0.25*(v00+v10+v01+v11);

    int sampleDim =
        int(ceil(sqrt(sampleCountF)));

    int sampleCount =
        sampleDim*sampleDim;

    uint yi =
        sampleIdx / uint(sampleDim);

    uint xi =
        sampleIdx - yi*uint(sampleDim);

    float x =
        (float(xi)+0.5)/sampleDim;

    float y =
        (float(yi)+0.5)/sampleDim;

    screenCoord =
        vec2(pixel00) + vec2(x,y);

    pixelSize =
        vec2(1.0/sampleDim);
}



bool GetTask(
    out vec2 lightUV,
    out uvec2 pixelCoord,
    out vec2 deltaUV,
    out uint lightID,
    out uint isReflective)
{
    uvec3 launchIndex = gl_GlobalInvocationID;
    uvec3 launchDimension = gl_NumWorkGroups * gl_WorkGroupSize;

    uint taskIdx = launchIndex.y * launchDimension.x + launchIndex.x;

    lightID = 0u;

    // -----------------------------
    // Get pixel position
    // -----------------------------
    uvec2 pixelPosI;
    uint sampleIdx; //local index for 4 pixels block

    vec2 screenCoord = vec2(0.0);
    deltaUV = vec2(0.0);
    pixelCoord = uvec2(0);

    if (!GetSamplePos(taskIdx, pixelPosI, sampleIdx)) 
        return false;

    // -----------------------------
    // Read light id map
    // -----------------------------
    // uvec4 lightInfo = texelFetch(
    //     LightIDTexture,
    //     ivec2(pixelPosI / LIGHT_ID_MAP_BLOCK_SIZE),
    //     0);

    // lightID = (lightInfo.x & 0x1ffu);
    // isReflective = (lightInfo.x >> 15);
       isReflective = 0;

    // -----------------------------
    // exact sample position
    // -----------------------------
    vec2 pixelSize; //n photons in one pixel so the pixelsize of one photon is 1/n

    GetRaySample(pixelPosI, sampleIdx, screenCoord, pixelSize);

    pixelCoord = uvec2(screenCoord); // [0,width]

    //lightUV = (screenCoord + RandomOffset * pixelSize - vec2(lightInfo.zw)) / float(lightInfo.y);

    //the photons start uv for specific light
    uint seed = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * 4096u;

    vec2 RandomOffset;
    RandomOffset.x = Random(seed);
    RandomOffset.y = Random(seed);

    lightUV =(screenCoord + RandomOffset * pixelSize) / float(LightMapSize.y);  //[0,width] -> [0,1]

    

    //deltaUV = pixelSize / float(lightInfo.y);
    deltaUV = pixelSize / float(LightMapSize.y); //now 1 light 1 tile

    // -----------------------------
    // init photon index
    // -----------------------------
    //pixelInfo[taskIdx].photonIdx = -1;

    return true;
}

vec3 SamplePointLight(vec2 lightUV)
{
    vec2 signUV = vec2(
        lightUV.x > 0.0 ? 1.0 : -1.0,
        lightUV.y > 0.0 ? 1.0 : -1.0
    );

    vec2 absUV = abs(lightUV);

    vec3 direction = vec3(
        lightUV.x,
        lightUV.y,
        absUV.x + absUV.y - 1.0
    );

    if(direction.z > 0.0)
    {
        direction.xy = (1.0 - absUV.yx) * signUV;
    }

    return normalize(direction);
}

vec3 SampleSpotLight(float cosLength, float sinLength, vec2 lightUV)
{
    vec2 lightOffset = sinLength * lightUV;
    return vec3(lightOffset, cosLength);
}

void InitFromLight(
uint lightID,
vec2 lightUV,
vec2 deltaUV,
out RayDesc ray,
out CausticsUnpackedPayload hitData,
inout vec3 dDdx,
inout vec3 dDdy)
{

int LightId = int(lightID);

vec3 color0 = GetColor(LightId);
vec3 lightNormal = GetNormal(LightId);
vec3 lightPosition = GetPosition(LightId);
float maxDistance = GetAttenuation(LightId);
int lightType = GetLightType(LightId);

//////////////////////////////////////////////////////////
// soft caustics
//////////////////////////////////////////////////////////

//no soft caustics
float Softness = 0;
vec2 SoftOffset = vec2(0, 0);

//////////////////////////////////////////////////////////
// Directional light
//////////////////////////////////////////////////////////

if(lightType == LIGHT_DIRECTIONAL)
{

vec3 lightDirZ = -lightNormal;

vec3 lightDirY =
normalize(vec3(lightDirZ.z,0.0,-lightDirZ.x));

vec3 lightDirX =
cross(lightDirY,lightDirZ);

lightDirZ =
normalize(
lightDirZ
+ SoftOffset.x * lightDirX
+ SoftOffset.y * lightDirY);

vec3 lightOrigin = GetPosition(LightId);

float LightWidth = GetWidth(LightId);
float LightHeight = GetHeight(LightId);

vec2 pixelSize =
deltaUV * vec2(LightWidth,LightHeight);

vec2 center =
vec2(
dot(lightOrigin,lightDirX)/LightWidth,
dot(lightOrigin,lightDirY)/LightHeight);

vec2 centerUV =
floor(center)
+ lightUV
+ round(fract(center)-lightUV);

ray.o =
lightDirZ * dot(lightOrigin,lightDirZ)
+ lightDirX * centerUV.x * LightWidth
+ lightDirY * centerUV.y * LightHeight;

hitData.color =
vec3(pixelSize.x * pixelSize.y);

hitData.nextDir = lightDirZ;

hitData.dDdx = vec3(0);
hitData.dDdy = vec3(0);

hitData.dPdx =
lightDirX * pixelSize.x * 2.0;

hitData.dPdy =
-lightDirY * pixelSize.y * 2.0;

dDdx =
lightDirX * Softness * 2.0;

dDdy =
-lightDirY * Softness * 2.0;

}

//////////////////////////////////////////////////////////
// Point light
//////////////////////////////////////////////////////////

else if(lightType == LIGHT_POINT)
{

lightUV = lightUV * 2.0 - 1.0; //[-1,1]

vec3 direction =
SamplePointLight(lightUV);

vec3 direction0 =
SamplePointLight(lightUV + vec2(deltaUV.x,0));

vec3 direction1 =
SamplePointLight(lightUV + vec2(0,deltaUV.y));

vec3 DirectionDx =
direction0 - direction;

vec3 DirectionDy =
direction1 - direction;

ray.o = lightPosition;

hitData.color =
vec3(deltaUV.x * deltaUV.y * 100.0);

hitData.dDdx = DirectionDx * 2.0;
hitData.dDdy = DirectionDy * 2.0;

DirectionDx = normalize(DirectionDx);
DirectionDy = normalize(DirectionDy);

hitData.nextDir =
normalize(direction
+ SoftOffset.x * DirectionDx
+ SoftOffset.y * DirectionDy);

hitData.dPdx = vec3(0);
hitData.dPdy = vec3(0);

dDdx =
DirectionDx * Softness * 2.0;

dDdy =
DirectionDy * Softness * 2.0;

}

//////////////////////////////////////////////////////////
// Spot light
//////////////////////////////////////////////////////////

else if(lightType == LIGHT_SPOT)
{

lightUV = lightUV.yx * 2.0 - 1.0;

float CosConeAngle = GetCosConeAngle(LightId);
float SinConeAngle =sqrt(1.0 - CosConeAngle * CosConeAngle);

vec3 SoftOffsetDir = vec3(CosConeAngle * SoftOffset, CosConeAngle);

vec3 direction = SampleSpotLight(CosConeAngle,SinConeAngle,lightUV) + SoftOffsetDir;

vec3 direction0 = SampleSpotLight(CosConeAngle,SinConeAngle, lightUV + vec2(deltaUV.x,0)) + SoftOffsetDir;

vec3 direction1 = SampleSpotLight(CosConeAngle,SinConeAngle, lightUV + vec2(0,deltaUV.y)) + SoftOffsetDir;

mat3 basis = GetTangentBasis(lightNormal);

direction = normalize(basis * direction);

direction0 = normalize(basis * direction0);

direction1 = normalize(basis * direction1);

vec3 DirectionDx = direction0 - direction;

vec3 DirectionDy = direction1 - direction;

float CosConeInnerAngle = GetHeight(LightId);

float strength = SpotAttenuation(-direction,lightNormal, vec2(CosConeAngle,CosConeInnerAngle));

ray.o = lightPosition;

hitData.color = vec3(deltaUV.x * deltaUV.y * 0.1 * strength);

hitData.nextDir = normalize(direction);

hitData.dDdx = DirectionDx * 2.0;
hitData.dDdy = DirectionDy * 2.0;

hitData.dPdx = vec3(0);
hitData.dPdy = vec3(0);

//SetContinue(hitData, strength>0 ? 1 : 0);

if(strength>0)
    hitData.continueFlag = 1;
else
    hitData.continueFlag = 0;


DirectionDx = normalize(DirectionDx);
DirectionDy = normalize(DirectionDy);

dDdx = DirectionDx * Softness * 2.0;
dDdy = DirectionDy * Softness * 2.0;

}

//////////////////////////////////////////////////////////
// Rect light
//////////////////////////////////////////////////////////
/*
else if(lightType == LIGHT_RECT)
{

lightUV = lightUV * 2.0 - 1.0;

vec3 lightDirZ = lightNormal;
vec3 lightDirX = GetdPdu(LightId);
vec3 lightDirY = GetdPdv(LightId);

vec3 lightOrigin = lightPosition;

vec2 LightDim = vec2(GetWidth(LightId),GetHeight(LightId));

vec2 pixelSize =
deltaUV * LightDim;

float cosAngle =
GetRectLightBarnCosAngle(LightId);

float tanAngle =
min(5.0,sqrt(1.0/(cosAngle*cosAngle)-1.0));

vec3 direction =
vec3(lightUV * tanAngle
+ SoftOffset * Softness,
1.0);

vec3 direction0 =
direction +
vec3(vec2(deltaUV.x,0)*tanAngle,0);

vec3 direction1 =
direction +
vec3(vec2(0,deltaUV.y)*tanAngle,0);

direction =
normalize(
lightDirX * direction.x +
lightDirY * direction.y +
lightDirZ * direction.z);

direction0 =
normalize(
lightDirX * direction0.x +
lightDirY * direction0.y +
lightDirZ * direction0.z);

direction1 =
normalize(
lightDirX * direction1.x +
lightDirY * direction1.y +
lightDirZ * direction1.z);

ray.o =
lightOrigin
+ lightDirX * lightUV.x * LightDim.x * 0.5
+ lightDirY * lightUV.y * LightDim.y * 0.5;

hitData.color =
vec3(pixelSize.x * pixelSize.y);

hitData.nextDir = direction;

hitData.dDdx = (direction0-direction)*2.0;
hitData.dDdy = (direction1-direction)*2.0;

hitData.dPdx =
lightDirX * pixelSize.x * 2.0;

hitData.dPdy =
lightDirY * pixelSize.y * 2.0;

dDdx =
lightDirX * Softness * 2.0;

dDdy =
lightDirY * Softness * 2.0;

}
*/
//////////////////////////////////////////////////////////
/// other
//////////////////////////////////////////////////////////
else
	{
		ray.TMax = 1e6;
		hitData.color = vec3(0.0);
		hitData.nextDir = vec3(0.0);
		hitData.dDdx = vec3(0.0);
		hitData.dDdy = vec3(0.0);
		hitData.dPdx = vec3(0.0);
		hitData.dPdy = vec3(0.0);
	}
//////////////////////////////////////////////////////////

ray.TMin = 0.01;
ray.TMax = maxDistance;

ray.d = hitData.nextDir;

hitData.color *= color0 * Intensity / (SplatSize * SplatSize);

}

/////////////////////////////////////////////////////////
/// Material
/////////////////////////////////////////////////////////



// -----------------------------
// 工具函数
// -----------------------------

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

float luminance(vec3 c)
{
    // 标准 Rec.709
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// -----------------------------
// Schlick Fresnel
// -----------------------------
vec3 FresnelSchlick(vec3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// -----------------------------
// 从 IOR 计算 F0（可选）
// -----------------------------
float IorToF0(float ior)
{
    float f = (1.0 - ior) / (1.0 + ior);
    return f * f;
}

// -----------------------------
// 主函数：计算 fresnel 概率
// -----------------------------
float ComputeFresnelProbability(vec3 specularColor, float cosTheta)
{
    float NoV = saturate(abs(cosTheta));

    vec3 F = FresnelSchlick(specularColor, NoV);

    // 转成标量概率（用于 isReflect）
    return luminance(F);
}

bool IsTotalInternalReflection(vec3 I, vec3 N, float eta)
{
    float cosI = clamp(dot(-I, N), -1.0, 1.0);
    float k = 1.0 - eta * eta * (1.0 - cosI * cosI);
    return k < 0.0;
}

void CalculateDNdx(
    vec3 dP1, vec3 dP2,
    vec3 dN1, vec3 dN2,
    vec3 N,
    vec3 dPdx,
    out vec3 dNdx)
{
    float P11 = dot(dP1, dP1);
    float P12 = dot(dP1, dP2);
    float P22 = dot(dP2, dP2);

    float Q1 = dot(dP1, dPdx);
    float Q2 = dot(dP2, dPdx);

    float delta = P11 * P22 - P12 * P12;

    float dudx = (Q1 * P22 - Q2 * P12) / delta;
    float dvdx = (Q2 * P11 - Q1 * P12) / delta;

    float n2 = dot(N, N);
    float n1 = sqrt(n2);

    dNdx = dudx * dN1 + dvdx * dN2;

    // project to tangent plane (unit normal constraint)
    dNdx = (n2 * dNdx - dot(N, dNdx) * N) / (n2 * n1);
}

//////////////////////////////////////////////////////////
/// Trace
/// /////////////////////////////////////////////////////


RayDesc world2Local(RayDesc r, mat4 model)
{
    vec3 e=r.o+r.d;

    vec4 localO=inverse(model)*vec4(r.o,1.0);
    localO/=localO.w;

    vec4 localE=inverse(model)*vec4(e,1.0);
    localE/=localE.w;

    RayDesc localR;
    localR.o=localO.xyz;
    localR.d=normalize(localE.xyz-localO.xyz);

    return localR;
}

bool ComputeSurfaceDerivatives(
    vec3 P0, vec3 P1, vec3 P2,
    vec2 UV0, vec2 UV1, vec2 UV2,
    out vec3 dPdu,
    out vec3 dPdv)
{
    vec3 edge1 = P1 - P0;
    vec3 edge2 = P2 - P0;

    vec2 duv1 = UV1 - UV0;
    vec2 duv2 = UV2 - UV0;

    float det = duv1.x * duv2.y - duv1.y * duv2.x;

    // ===== degenerate UV handling =====
    if (abs(det) < 1e-8)
    {
        // fallback: 构造一个几何正交基（不依赖UV）
        vec3 N = normalize(cross(edge1, edge2));

        // 找一个不平行的向量
        vec3 tangent = normalize(abs(N.x) > 0.5 ? vec3(N.y, -N.x, 0.0) 
                                                : vec3(0.0, N.z, -N.y));
        vec3 bitangent = normalize(cross(N, tangent));

        dPdu = tangent;
        dPdv = bitangent;

        return false; // 标记 fallback
    }

    float invDet = 1.0 / det;

    dPdu = ( duv2.y * edge1 - duv1.y * edge2) * invDet;
    dPdv = (-duv2.x * edge1 + duv1.x * edge2) * invDet;

    return true;
}

void ComputeNormalDerivatives(
    vec3 N0, vec3 N1, vec3 N2,
    vec2 UV0, vec2 UV1, vec2 UV2,
    out vec3 dNdu,
    out vec3 dNdv)
{
    vec3 edgeN1 = N1 - N0;
    vec3 edgeN2 = N2 - N0;

    vec2 duv1 = UV1 - UV0;
    vec2 duv2 = UV2 - UV0;

    float det = duv1.x * duv2.y - duv1.y * duv2.x;

    if (abs(det) < 1e-8)
    {
        dNdu = vec3(0.0);
        dNdv = vec3(0.0);
        return;
    }

    float invDet = 1.0 / det;

    dNdu = ( duv2.y * edgeN1 - duv1.y * edgeN2) * invDet;
    dNdv = (-duv2.x * edgeN1 + duv1.x * edgeN2) * invDet;
}


Plane planeConstruct(int axis, float dis, vec3 color)
{
    Plane p;
    p.axis=axis;
    p.distanceFromO=dis;
    p.color=color;
    p.matType=MAT_DIFFUSE;

    return p;
}

void initWall()
{
    planes[0]=planeConstruct(AXIS_X, 1.5, vec3(0.8,0.3,0.3));
    planes[1]=planeConstruct(AXIS_X, -1.5, vec3(0.3,0.8,0.3));
    planes[2]=planeConstruct(AXIS_Y, 1.5, vec3(0.3,0.3,0.8));
    planes[3]=planeConstruct(AXIS_Y, -1.5, vec3(0.75));
    planes[4]=planeConstruct(AXIS_Z, -5, vec3(0.75));
}


bool pIntersect(RayDesc r, Plane p, out float t, out vec3 normal)
{
    t=-1.0;
    if(p.axis == AXIS_X) {
  	 	if(r.d.x != 0.0){                         
    		t=(p.distanceFromO - r.o.x) / r.d.x;
            normal=vec3(-sign(p.distanceFromO),0.0,0.0);
        }
  	}
  	else if(p.axis == AXIS_Y) {
  		if(r.d.y != 0.0){
  			t=(p.distanceFromO - r.o.y) / r.d.y;
            normal=vec3(0.0,-sign(p.distanceFromO),0.0);
        }
  	}
  	else if(p.axis == AXIS_Z) {
  		if(r.d.z != 0.0){
  			t=(p.distanceFromO - r.o.z) / r.d.z;
            normal=vec3(0.0,0.0,-sign(p.distanceFromO));
        }
  	}
  	
    if(t<=0.01)
        return false;
    else
        return true;
}

bool hitPlane(RayDesc r, inout HitRecord rec)
{
    float minT=-1.0f;
    float tempT=-1.0f;
    vec3 tempN;
    for(int i=0;i<5;i++)
    {
        if(pIntersect(r,planes[i],tempT,tempN))
        {
            if(tempT > 0.0 && (minT <= 0.0 || tempT < minT))
            {
                minT=tempT;

                rec.t=minT;
                rec.color=planes[i].color;
                rec.normal=tempN;
                rec.matType=MAT_DIFFUSE;
            }
           
        }
    }
    if(minT>0){
       
        return true;
    }
    else
        return false;
}

bool ifHitTri(vec3 v1, vec3 v2, vec3 v3, vec3 n, RayDesc r, out float t)
{
     //和三角形所在的面判断交点
    t=dot(v1-r.o,n)/dot(r.d,n);

    if(t<=0.00001)
    {
        return false;
    }

    vec3 p=r.o+t*r.d;
    //然后判断是否在三角形内
    vec3 c1=cross(v2-v1,p-v1);
    vec3 c2=cross(v3-v2,p-v2);
    vec3 c3=cross(v1-v3,p-v3);

    if(dot(c1,c2)>0 && dot(c2,c3)>0)
    {
       return true;
    }
    else
    {
       return false;
    }

    
}


bool hitGlasses(RayDesc r, inout HitRecord rec)
{
    float minT=-1.0f;
    int minObjIdx=-1;
    int minIndicesIdx=-1;
    vec4 minInfo=vec4(-1.0f);
    vec3 minNormal=vec3(0.0f);
    mat4 minModel=mat4(0.0f);
    vec3 minDPdu=vec3(0.0f);
    vec3 minDPdv=vec3(0.0f);
    vec3 minDNdu=vec3(0.0f);
    vec3 minDNdv=vec3(0.0f);
    float tempT=-1.0f;

    for(int i=0;i<objNum;i++)
    {
        //for glasses[i]
        //get index number
        vec4 info=texture(verticesTex,vec2(0.5, i+0.5)/verticesTexSize);
        float indicesNum=info.g;
        //获得model矩阵
        mat4 model=mat4(texture(verticesTex,vec2(2.5, i+0.5)/verticesTexSize),
                        texture(verticesTex,vec2(3.5, i+0.5)/verticesTexSize),
                        texture(verticesTex,vec2(4.5, i+0.5)/verticesTexSize),
                        texture(verticesTex,vec2(5.5, i+0.5)/verticesTexSize));

        //将ray 转换object space
        RayDesc localR=world2Local(r, model);
       

        //3个一组读取顶点信息进行碰撞检测
        float localMinT=-1.0f; // 对应obj Space 的t
        int localMinIdx=-1;
        vec3 localMinN=vec3(0.0f);
        vec3 localv1=vec3(0.0f);
        vec3 localv2=vec3(0.0f);
        vec3 localv3=vec3(0.0f);
        vec3 localn=vec3(0.0f);
        vec2 localuv1=vec2(0.0f);
        vec2 localuv2=vec2(0.0f);
        vec2 localuv3=vec2(0.0f);
        for(int j=0;j<indicesNum;j+=3)
        {
            float x1=6+j*2+0.5;
            float y=i+0.5;
            vec4 v1=texture(verticesTex,vec2(x1,y)/verticesTexSize);
            vec4 v2=texture(verticesTex,vec2(x1+2,y)/verticesTexSize);
            vec4 v3=texture(verticesTex,vec2(x1+4,y)/verticesTexSize);

            //采样法线
            vec4 n1=texture(verticesTex,vec2(x1+1,y)/verticesTexSize);
            vec4 n2=texture(verticesTex,vec2(x1+3,y)/verticesTexSize);
            vec4 n3=texture(verticesTex,vec2(x1+5,y)/verticesTexSize);

            vec2 uv1=vec2(v1.w,n1.w);
            vec2 uv2=vec2(v2.w,n2.w);
            vec2 uv3=vec2(v3.w,n3.w);

            if(ifHitTri(v1.xyz, v2.xyz, v3.xyz, n1.xyz, localR, tempT))
            {
                if(localMinT<=-1.0f || tempT<localMinT)
                {
                    localMinT=tempT;
                    localMinIdx=j;
                    localMinN=n1.xyz;

                    localv1=v1.xyz;
                    localv2=v2.xyz;
                    localv3=v3.xyz;
                    localn =n1.xyz; 
                    localuv1=uv1;
                    localuv2=uv2;
                    localuv3=uv3;
       
            
                }
            }
        }

        //将localMinT 转换成 world space 和 minT 做比较 并更新 minT 和 index
        if(localMinT>0.0f) //该物体存在碰撞才有意义
        {
            vec3 localP=localR.o+localMinT*localR.d;
            vec4 p=model*vec4(localP,1.0f);
            p/=p.w;
            float tempT = dot(p.xyz - r.o, r.d);
            if(minT<=0.0f || tempT<minT)
            {
                minT=tempT; //world
                minObjIdx=i;
                minIndicesIdx=localMinIdx;
                minInfo=info;
                minNormal=localMinN; //local
                minModel=model;      //local

                ComputeSurfaceDerivatives(localv1, localv2,localv3, localuv1, localuv2,localuv3, minDPdu, minDPdv);
                ComputeNormalDerivatives(localn,localn,localn,localuv1,localuv2,localuv3,minDNdu,minDNdv);

            }
        }
    }

    if(minT <=0.0000001f) //太小说明失败，不用特意更新了
        return false;

    //根据 minT 和 index 采样并插值计算 normal color 等 并填充 hitrecord
    if(rec.t<=0.0f || minT<rec.t) //此时rec 中没有碰撞信息， 或者现在的碰撞信息 比record 中的距离更近，就更新
    {
        rec.t=minT;
        rec.color=texture(verticesTex,vec2(1.5,minObjIdx+0.5)/verticesTexSize).xyz;
	    rec.normal = normalize(mat3(transpose(inverse(minModel))) * minNormal);
	    rec.matType=minInfo.z;
        rec.dPdu=minDPdu;
        rec.dPdv=minDPdv;
        rec.dNdu=minDNdu;
        rec.dNdv=minDNdv;
        return true;
    }

    return false;
}

bool ifHit(RayDesc r, inout HitRecord rec)
{
    //判断最外面的墙
    hitPlane(r,rec);

    hitGlasses(r,rec);

    if(rec.t>0.0f)
        return true;
    else
        return false;
}

void UpdateTransferRayDifferential(
    vec3 D,
    float t,
	vec3 N,
	inout vec3 dPdx,
	vec3 dDdx)
{
	
	float dtdx = -1 * dot(dPdx + t * dDdx, N) / dot(D, N);
	dPdx = dPdx + t * dDdx + dtdx * D;
}

void UpdateReflectRayDifferential(
    vec3 D,
    vec3 N,
    vec3 dNdx,
    inout vec3 dDdx)
{
    float dDNdx = dot(dDdx, N) + dot(D, dNdx);

    dDdx = dDdx - 2.0 * (dot(D, N) * dNdx + dDNdx * N);
}

void UpdateRefractRayDifferential(
    vec3 D,
    vec3 R,
    vec3 N,
    float eta,
    vec3 dPdx,
    vec3 dNdx,
    inout vec3 dDdx)
{
    float DN = dot(D, N);
    float RN = dot(R, N);

    
    float safeRN = max(abs(RN), 1e-6);

    float mu = eta * DN - RN;

    float dDNdx = dot(dDdx, N) + dot(D, dNdx);

    float dmudx = (eta - eta * eta * DN / safeRN) * dDNdx;

    dDdx = eta * dDdx - (mu * dNdx + dmudx * N);
}

bool TracePhoton(inout RayDesc ray, inout CausticsUnpackedPayload hitData)
{
    
    //find intersection
    HitRecord rec;
    rec.t=-1.0;
    if(ifHit(ray, rec))
    {
        //generate new dir according to the Material Property

        //get hit position and Material propperty
       
        int matType=int(rec.matType);
        hitData.hitT =rec.t;


        //Transfer differencial
        UpdateTransferRayDifferential(ray.d, rec.t, rec.normal, hitData.dPdx, hitData.dDdx);
        UpdateTransferRayDifferential(ray.d, rec.t, rec.normal, hitData.dPdy, hitData.dDdy);

        //new dir according to Material
        if(matType == MAT_DIFFUSE)
        {
            //end the photon trace
            hitData.continueFlag=0;

            hitData.color *= dot(-ray.d, rec.normal);

		    hitData.nextDir = rec.normal;

            return true;
        }
        else if(matType == MAT_SPECULAR || matType == MAT_REFRACT)
        {
            //decide whether stop or reflect or refract
            hitData.continueFlag=1;
            bool isReflect=true;

            float footprintScale = 1.0;
            float ior = 1.5; //for glasses

            float eta = 1.0 / ior;

            float cosVal=dot(ray.d,rec.normal);
            
            
            vec3 F=rec.color;

            if(matType==MAT_REFRACT)
            {
                
                //monte carlo random reflect

                if(cosVal>0.0) //from inner to otter
                {
                    rec.normal*=-1.0;
                    eta=1.0/eta;
                }
    
                // float F0_scalar = IorToF0(ior);   // ≈ 0.04
                // vec3 SpecularColor = vec3(F0_scalar);

                // float fresnel = ComputeFresnelProbability(SpecularColor, abs(cosVal));

                // uvec2 pixel = gl_GlobalInvocationID.xy;
                // uint seed = pixel.x + pixel.y * 4096u + uint(fract(rec.minT) * 100000.0);
                // float rand = Random(seed);  // 0~1

                //isReflect = rand < fresnel;

                //footprintScale = isReflect ? 1.0 / sqrt(fresnel) : 1.0 / sqrt(1 - fresnel);

                isReflect = false;

                if(!isReflect && IsTotalInternalReflection(ray.d, rec.normal, eta))
                {
                    footprintScale = 1;
                    isReflect = true;
                    F = vec3(1.0);
				        
                }
                
            }


            vec3 dNdx = vec3(0.0);
            vec3 dNdy = vec3(0.0);
		    hitData.dPdx *= footprintScale;
		    hitData.dPdy *= footprintScale;
		    CalculateDNdx(rec.dPdu, rec.dPdv, rec.dNdu, rec.dNdv, rec.normal, hitData.dPdx, dNdx);
		    CalculateDNdx(rec.dPdu, rec.dPdv, rec.dNdu, rec.dNdv, rec.normal, hitData.dPdy, dNdy);

            if (cosVal > 0)
		    {
			    dNdx *= -1;
			    dNdy *= -1;
		    }

            if(isReflect)
            {
                
                UpdateReflectRayDifferential(ray.d, rec.normal, dNdx, hitData.dDdx);
			    UpdateReflectRayDifferential(ray.d, rec.normal, dNdy, hitData.dDdy);
			    hitData.nextDir = normalize(reflect(ray.d, rec.normal));
                hitData.color = F * hitData.color;
                
            }
            else
            {
                //dispersion

                //refract differencial
                hitData.nextDir=normalize(refract(ray.d,rec.normal ,eta));
			    UpdateRefractRayDifferential(ray.d, hitData.nextDir, rec.normal, eta, hitData.dPdx, dNdx, hitData.dDdx);
			    UpdateRefractRayDifferential(ray.d, hitData.nextDir, rec.normal, eta, hitData.dPdy, dNdy, hitData.dDdy);

                // color glass filtering
                vec3 glassColor = rec.color;   // color
                float transparency = 0.2; // 1 is opacity， 0 is full transparent
                vec3 filterClr = mix(glassColor, vec3(1.0), transparency);
                hitData.color *= filterClr; // for photons

            }



            return true;

        }

         

    }

    return false;
    

}

///////////////////////////////////////////
/// Store Photon
///////////////////////////////////////////


bool IsInFrustumXY(vec2 p)
{
    return all(lessThan(abs(p), vec2(1.0)));
}

// 
bool IsInFrustumZ(vec4 p)
{
    
    if (p.z < 0.0 || p.z > 1.0)
        return false;

    // 边缘区域直接通过（避免 depth 误差）
    if (any(greaterThan(abs(p.xy), vec2(0.99))))
        return true;

    // NDC → 像素坐标
    vec2 uv = (p.xy + 1.0) * 0.5 * ViewportDim; //->[0,1]

    ivec2 iuv = ivec2(uv);

    // 读取 depth（等价 HLSL .Load）
    float depth = texelFetch(SceneDepthBuffer, iuv, 0).r * 2.0 - 1.0; //->[0,1]

    // occlusion test
    return abs(depth - p.z) < 0.01;
}


float GetArea(vec3 dPdx, vec3 dPdy)
{
    vec3 areaVector = cross(dPdx, dPdy);
    return length(areaVector);
}

float GetPhotonScreenArea(
    vec3 posW,
    vec3 dPdx,
    vec3 dPdy,
    out vec3 screenCoord,
    out bool inFrustum)
{
    // -----------------------------------
    // scale footprint
    // -----------------------------------
    dPdx *= SplatSize;
    dPdy *= SplatSize;

    // -----------------------------------
    // project real positions (correct)
    // -----------------------------------
    vec4 c0 = ViewProjectionMatrix * vec4(posW, 1.0);
    vec4 cx = ViewProjectionMatrix * vec4(posW + dPdx, 1.0);
    vec4 cy = ViewProjectionMatrix * vec4(posW + dPdy, 1.0);

    // behind camera
    if(c0.w <= 0.00001 ||
       cx.w <= 0.00001 ||
       cy.w <= 0.00001)
    {
        inFrustum = false;
        screenCoord = vec3(0.0);
        return 0.0;
    }

    // clip -> ndc
    c0 /= c0.w;
    cx /= cx.w;
    cy /= cy.w;

    // -----------------------------------
    // screen differential
    // -----------------------------------
    vec2 dx = (cx.xy - c0.xy) * ViewportDim;
    vec2 dy = (cy.xy - c0.xy) * ViewportDim;

    // -----------------------------------
    // 4 corners footprint
    // -----------------------------------
    vec2 p0 = c0.xy + dx / ViewportDim + dy / ViewportDim;
    vec2 p1 = c0.xy + dx / ViewportDim - dy / ViewportDim;
    vec2 p2 = c0.xy - dx / ViewportDim + dy / ViewportDim;
    vec2 p3 = c0.xy - dx / ViewportDim - dy / ViewportDim;

    // -----------------------------------
    // frustum test
    // any corner overlaps screen
    // -----------------------------------
    bool insideXY =
        IsInFrustumXY(p0) ||
        IsInFrustumXY(p1) ||
        IsInFrustumXY(p2) ||
        IsInFrustumXY(p3) ||
        IsInFrustumXY(c0.xy);

    bool insideZ = (c0.z >= -1.0 && c0.z <= 1.0);

    inFrustum = insideXY && insideZ;

    // -----------------------------------
    // area in pixels
    // -----------------------------------
    float area = abs(dx.x * dy.y - dx.y * dy.x);

    area /= (SplatSize * SplatSize);

    // -----------------------------------
    // output center
    // -----------------------------------
    screenCoord = c0.xyz;
    screenCoord.xy = screenCoord.xy * 0.5 + 0.5;

    return area;
}



void PackPhoton(
    vec3 posW,
    vec3 color,
    vec3 dPdx,
    vec3 dPdy,
    vec3 dir,
    out Photon photon)
{
    photon.posW = vec4(posW, 1.0);
    photon.color = vec4(color, 1.0);
    photon.dPdx = vec4(dPdx, 0.0);
    photon.dPdy = vec4(dPdy, 0.0);
    photon.dir  = vec4(dir, 0.0);
}

float GetVariance(vec3 screenCoord)
{
    
    vec2 uv = screenCoord.xy;

    // ->[0,width]
    ivec2 iuv = ivec2(uv * ViewportDim);
    iuv = clamp(iuv, ivec2(0), ivec2(ViewportDim) - 1);

    // 等价 HLSL Texture2DSample + point sampler
    float variance = texelFetch(VarianceTexture, iuv, 0).w;
    return variance;
}

void StorePhoton(RayDesc ray, CausticsUnpackedPayload hitData, uvec2 pixelCoord, float areaFactor)
{
	bool bInFrustum;
	vec3 posW = ray.o;
	vec3 dPdx=hitData.dPdx;
    vec3 dPdy=hitData.dPdy;

	float area = GetArea(dPdx, dPdy);

	vec3 color = hitData.color / area;
	vec3 screenCoord;

    
	float pixelArea = GetPhotonScreenArea(posW, dPdx, dPdy, screenCoord, bInFrustum);
	

	if (bInFrustum)
	{
		// For dispersion
		

		uint pixelLoc = pixelCoord.y* LightMapSize.x + pixelCoord.x;

		bool storePhoton = (pixelArea < MaxScreenRadius * MaxScreenRadius); 
		uint oldV;

		if (storePhoton)
        {
            uint instanceIdx = atomicCounterIncrement(photonCounter);

            if (dot(cross(dPdx, dPdy), hitData.nextDir) < 0.0)
            {
                dPdy *= -1.0;
            }

            Photon photon;
            PackPhoton(posW, color, dPdx, dPdy, -ray.d, photon);

            photons[instanceIdx] = photon;
        }

		// Update pixel info buffer
		float variance = GetVariance(screenCoord);

        pixelArea = clamp(pixelArea, 1.0, MaxScreenRadius * MaxScreenRadius);

        pixelArea /= (areaFactor * areaFactor);

        atomicAdd(pixelInfo[pixelLoc].screenArea, int(pixelArea));
        atomicAdd(pixelInfo[pixelLoc].screenAreaSq, int(variance * 255.0));
        atomicAdd(pixelInfo[pixelLoc].count, 1);
	}
}



void main()
{
    //init wall
    initWall();

    vec2 lightUV;       //light emit coord for init photon
    uvec2 pixelCoord;   //light emit pos for store photons
    vec2 deltaUV;       //one photon's pixel size in the whole light emit tex
    uint lightID;       //which light emits this photon
    uint isReflective;

    //get photon pos
    if(!GetTask(lightUV, pixelCoord, deltaUV, lightID, isReflective))
        return;


    //init photons
    RayDesc ray;
	CausticsUnpackedPayload hitData;
    hitData.continueFlag = 1;
	vec3 dDdx = vec3(0.0), dDdy = vec3(0.0); 


    // ------------------------ debug ---------------------------------

    // ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

    // if(pixel.x >= LightMapSize.x || pixel.y >= LightMapSize.y)
    //     return;

    // lightUV = (vec2(pixel) + 0.5) / vec2(LightMapSize);

    //----------------------debug ---------------------------planes----------

    InitFromLight(lightID, lightUV, deltaUV, ray, hitData, dDdx, dDdy);

    float colorIntensity = max(hitData.color.r, max(hitData.color.g, hitData.color.b));
    hitData.color /= colorIntensity;

	float areaFactor = 1;

    //area
    float oldArea = GetArea(hitData.dDdx, hitData.dDdy) + 1e-5;
	hitData.dDdx += dDdx;
	hitData.dDdy += dDdy;
	float newArea = GetArea(hitData.dDdx, hitData.dDdy) + 1e-5;
	areaFactor = newArea / oldArea;

    //Photon Trace

	float hitTBudget = ray.TMax;

    int depth = 0;

	for (depth = 0; depth < MaxDepth && hitData.continueFlag==1 && ray.TMax > ray.TMin; depth++)
	{
		ray.d = hitData.nextDir;

		if(!TracePhoton(ray, hitData))
        {
            break;
        }

		float area = (dot(hitData.dPdx, hitData.dPdx) + dot(hitData.dPdy, hitData.dPdy)) * 0.5;
		if (hitData.continueFlag ==1 && dot(hitData.color, vec3(0.299, 0.587, 0.114)) * colorIntensity < MidCullColorThreshold * area)
		{
			hitData.continueFlag = 0;
			hitData.color = vec3(0.0);
		}

		ray.o = ray.o + ray.d * (hitData.hitT);
		hitTBudget -= hitData.hitT;
		ray.TMax = min(MaxBounceDistance, hitTBudget);

	}

    // Store photon
	if (any(greaterThan(hitData.color, vec3(0.0))) && depth > 1 && hitData.continueFlag==0)
    {
        // Store photons to photon buffer, and store statistics to pixel info buffer
        hitData.color *= colorIntensity;
       
        StorePhoton(ray, hitData, pixelCoord, areaFactor);
    }


}