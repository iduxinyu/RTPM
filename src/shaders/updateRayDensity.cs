//compute shader update ray density
#version 430

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// ---------- Textures ----------
layout(binding = 0) uniform sampler2D RayDensitySourceTexture;
layout(binding = 1, r32f) uniform image2D RayDensityTargetTexture;

// ---------- SSBO ----------
struct PixelInfo {
    int count;
    int screenArea;
    int screenAreaSq;
};

layout(std430, binding = 2) buffer PixelInfoBuffer {
    PixelInfo pixelInfo[];
};

// ---------- Params ----------
layout(std140, binding = 3) uniform Params {
    ivec2 CoarseDim;
    float MinPhotonPixelSize;
    float VarianceGain;
    float SmoothWeight;
    int MaxTaskPerPixel;
    float UpdateSpeed;
};


// ---------- Helper ----------
void GetAreaValue(ivec2 pos, out float avgArea, out float avgArea2)
{
    int idx = CoarseDim.x * pos.y + pos.x;
    uint count = pixelInfo[idx].count;
    avgArea  = pixelInfo[idx].screenArea   / float(count + 1u);
    avgArea2 = pixelInfo[idx].screenAreaSq / float(count + 1u);


}

// ---------- Main ----------
void main()
{
    //thread ID
    ivec2 tid = ivec2(gl_GlobalInvocationID.xy);

    if (tid.x >= CoarseDim.x || tid.y >= CoarseDim.y)
        return;

    //get footprint area avg and sq for updating ray density 
    float area;
    float avgColorVariance;
    GetAreaValue(tid, area, avgColorVariance);

    float targetArea = MinPhotonPixelSize * MinPhotonPixelSize;

    float oldDensity = texelFetch(RayDensitySourceTexture, tid, 0).r;

    float weight = SmoothWeight * UpdateSpeed;

    oldDensity += texelFetch(RayDensitySourceTexture, tid + ivec2( 1,  0), 0).r * weight;
    oldDensity += texelFetch(RayDensitySourceTexture, tid + ivec2( 0,  1), 0).r * weight;
    oldDensity += texelFetch(RayDensitySourceTexture, tid + ivec2(-1,  0), 0).r * weight;
    oldDensity += texelFetch(RayDensitySourceTexture, tid + ivec2( 0, -1), 0).r * weight;
    oldDensity /= (1.0 + weight * 4.0);

    float newDensity =
        oldDensity * area / targetArea +
        min(1000.0, avgColorVariance * VarianceGain);

    newDensity = mix(oldDensity, newDensity, UpdateSpeed);

    if (abs(newDensity - oldDensity) < 0.05 * oldDensity)
        newDensity = oldDensity;

    newDensity = clamp(newDensity, 0.1, float(MaxTaskPerPixel));

    imageStore(
    RayDensityTargetTexture,
    tid,
    vec4(newDensity, newDensity - oldDensity, 0.0, 0.0));

   
}