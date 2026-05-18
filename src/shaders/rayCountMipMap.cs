#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform int MipLevel;


layout(std430, binding = 1) buffer RayCountBuffer
{
    uvec4 RayCountQuadTree[];
};

layout(std430, binding = 2) buffer MipOffsets
{
    uint mipOffset[];
};

int GetMipSize(int level)
{
    return 1<<level;
}

uint GetTextureOffset(ivec2 threadIdx, int MipLevel)
{
    int mipW = GetMipSize(MipLevel);

    return mipOffset[MipLevel] + threadIdx.y * mipW + threadIdx.x;

}

void main()
{
    ivec2 threadIdx = ivec2(gl_GlobalInvocationID.xy);

    int mipDim = GetMipSize(MipLevel);

    if (threadIdx.x >= mipDim || threadIdx.y >= mipDim)
        return;

    ivec2 pixel00 = threadIdx * 2;
    int nextMipLevel = MipLevel + 1;

    uvec4 count00 = RayCountQuadTree[GetTextureOffset(pixel00 + ivec2(0,0), nextMipLevel)];
    uvec4 count10 = RayCountQuadTree[GetTextureOffset(pixel00 + ivec2(1,0), nextMipLevel)];
    uvec4 count01 = RayCountQuadTree[GetTextureOffset(pixel00 + ivec2(0,1), nextMipLevel)];
    uvec4 count11 = RayCountQuadTree[GetTextureOffset(pixel00 + ivec2(1,1), nextMipLevel)];

    uvec4 value;

    value.x = count00.w;
    value.y = value.x + count10.w;
    value.z = value.y + count01.w;
    value.w = value.z + count11.w;

    RayCountQuadTree[GetTextureOffset(threadIdx, MipLevel)] = value;
}