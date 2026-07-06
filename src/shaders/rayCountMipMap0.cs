#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform int MipLevel;

layout(binding = 0) uniform sampler2D RayDensityTexture;

layout(std430, binding = 1) buffer RayCountBuffer
{
    uvec4 RayCountQuadTree[];
};

layout(std430, binding = 2) buffer MipOffsets
{
    uint mipOffset[];
};

uint GetSampleCount(float v00, float v10, float v01, float v11)
{
    float sampleCountF = 0.25 * (v00 + v10 + v01 + v11);
    int sampleDim = int(ceil(sqrt(sampleCountF)));
    uint sampleCount = uint(sampleDim * sampleDim);
    return sampleCount;
}

uint GetTextureOffset(ivec2 threadIdx, int MipLevel)
{
    int mipW = 1 << MipLevel; //  miplevel=0: mipW=1 ;   mipLevel=1: mipW=2;

    return mipOffset[MipLevel] + threadIdx.y * mipW + threadIdx.x;

}

void main()
{
    ivec2 threadIdx = ivec2(gl_GlobalInvocationID.xy);

    ivec2 pixel00 = threadIdx * 2;

    float v00 = texelFetch(RayDensityTexture, pixel00 + ivec2(0,0), 0).r;
    float v10 = texelFetch(RayDensityTexture, pixel00 + ivec2(1,0), 0).r;
    float v20 = texelFetch(RayDensityTexture, pixel00 + ivec2(2,0), 0).r;

    float v01 = texelFetch(RayDensityTexture, pixel00 + ivec2(0,1), 0).r;
    float v11 = texelFetch(RayDensityTexture, pixel00 + ivec2(1,1), 0).r;
    float v21 = texelFetch(RayDensityTexture, pixel00 + ivec2(2,1), 0).r;

    float v02 = texelFetch(RayDensityTexture, pixel00 + ivec2(0,2), 0).r;
    float v12 = texelFetch(RayDensityTexture, pixel00 + ivec2(1,2), 0).r;
    float v22 = texelFetch(RayDensityTexture, pixel00 + ivec2(2,2), 0).r;


    uint count00 = GetSampleCount(v00, v10, v01, v11);
    uint count10 = count00 + GetSampleCount(v10, v20, v11, v21);
    uint count01 = count10 + GetSampleCount(v01, v11, v02, v12);
    uint count11 = count01 + GetSampleCount(v11, v21, v12, v22);

    uint offset = GetTextureOffset(threadIdx, MipLevel);

    RayCountQuadTree[offset] = uvec4(count00, count10, count01, count11);
}