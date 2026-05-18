#version 430


layout(local_size_x = 16, local_size_y = 16) in;


// ===== uniforms =====
uniform mat4 ReprojectionMatrix;
uniform mat4 InverseProjectionMatrixLast;

uniform int Enable;
uniform ivec2 CausticsDim;
uniform ivec2 GBufferDim;

uniform float BlendWeight;
uniform float NormalKernel;
uniform float DepthKernel;
uniform float ColorKernel;

// ===== textures =====
layout(binding = 0) uniform sampler2D DepthTextureThis;
layout(binding = 1) uniform sampler2D DepthTextureLast;

layout(binding = 2) uniform sampler2D NormalTextureThis;
layout(binding = 3) uniform sampler2D NormalTextureLast;

layout(binding = 4) uniform sampler2D CausticsTextureThis;
layout(binding = 5) uniform sampler2D CausticsTextureLast;

//output
layout(rgba32f, binding = 6) uniform image2D BlendedResult;

// helper
float ToViewSpace(mat4 invProj, float depth)
{
    return (invProj[2][2] * depth + invProj[3][2]) /
           (invProj[2][3] * depth + invProj[3][3]);
}


void main()
{
    //get current info
    ivec2 causticsPixelPos = ivec2(gl_GlobalInvocationID.xy);

    if (any(greaterThanEqual(causticsPixelPos, CausticsDim)))
        return;

    vec2 uv = (vec2(causticsPixelPos) + 0.5) / vec2(CausticsDim);
    ivec2 gBufferPixelPos = ivec2(uv * vec2(GBufferDim));

    float depth = texelFetch(DepthTextureThis, gBufferPixelPos, 0).r;

    if (depth == 0.0 || depth == 1.0)
    {
        imageStore(BlendedResult, causticsPixelPos, vec4(0));
        return;
    }

    vec4 color = texelFetch(CausticsTextureThis, causticsPixelPos, 0);
    vec3 normal = texelFetch(NormalTextureThis, gBufferPixelPos, 0).rgb * 2.0 - 1.0;


    // ===== reprojection =====
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 ndcLast = ReprojectionMatrix * ndc;
    ndcLast.xyz /= ndcLast.w;

    vec2 uvLast = ndcLast.xy *0.5+0.5;

    ivec2 pixelPosLast = ivec2(uvLast * vec2(CausticsDim));
    ivec2 gBufferPixelPosLast = ivec2(uvLast * vec2(GBufferDim));

    ivec2 CausticsPixelPosLast = pixelPosLast;

    // ===== failure case =====
    if (any(greaterThanEqual(abs(ndcLast.xy), vec2(0.99))))
    {
        CausticsPixelPosLast = clamp(
            CausticsPixelPosLast,
            ivec2(vec2(CausticsDim) * 0.01),
            ivec2(vec2(CausticsDim) * 0.99)
        );

        float varianceLast = texelFetch(CausticsTextureLast, CausticsPixelPosLast, 0).a;
        imageStore(BlendedResult, causticsPixelPos, vec4(color.rgb, varianceLast));
        return;
    }

    float depthLast = texelFetch(DepthTextureLast, gBufferPixelPosLast, 0).r;
    vec3 normalLast = texelFetch(NormalTextureLast, gBufferPixelPosLast, 0).rgb * 2.0 - 1.0;

    vec4 colorLast = textureLod(CausticsTextureLast, uvLast, 0);

    // ===== variance smoothing =====
    float varianceSmoothWeight = 0.1;

    float varianceLast = colorLast.a;
    varianceLast += texelFetch(CausticsTextureLast, CausticsPixelPosLast + ivec2(1,0), 0).a * varianceSmoothWeight;
    varianceLast += texelFetch(CausticsTextureLast, CausticsPixelPosLast + ivec2(-1,0), 0).a * varianceSmoothWeight;
    varianceLast += texelFetch(CausticsTextureLast, CausticsPixelPosLast + ivec2(0,1), 0).a * varianceSmoothWeight;
    varianceLast += texelFetch(CausticsTextureLast, CausticsPixelPosLast + ivec2(0,-1), 0).a * varianceSmoothWeight;

    varianceLast /= (varianceSmoothWeight * 4.0 + 1.0);

    // ===== consistency =====
    float viewDepthLastReproj = ToViewSpace(InverseProjectionMatrixLast, ndcLast.z);
    float viewDepthLast = ToViewSpace(InverseProjectionMatrixLast, depthLast);

    float depthDiff = abs(viewDepthLast - viewDepthLastReproj) / DepthKernel;
    float normalDiff = (1.0 - clamp(dot(normal, normalLast), 0.0, 1.0)) / NormalKernel * 1000.0;

    float weight = 0.0;

    if (Enable != 0 && depthLast < 1.0)
    {
        weight = BlendWeight * exp(-(normalDiff * normalDiff + depthDiff * depthDiff));

        float edgeFade = 1.0 - clamp(max(abs(ndcLast.x), abs(ndcLast.y)) - 0.95, 0.0, 1.0) / (1.0 - 0.95);
        weight *= edgeFade;
    }

    vec3 newColor = clamp(color.rgb * (1.0 - weight) + colorLast.rgb * weight, 0.0, 1e10);

    vec3 deltaColor = newColor - colorLast.rgb;

    float varianceWeight = exp(-(normalDiff * normalDiff + depthDiff * depthDiff));
    float deltaLength = length(deltaColor);
    float deltaLuminance = deltaLength * varianceWeight;

    float newVariance = deltaLuminance / (dot(colorLast.rgb, vec3(1.0)) + 0.01);
    newVariance = varianceLast + min(newVariance - varianceLast, 0.05);

    float varianceSpeed = 0.95;
    float weightedVariance = mix(newVariance, varianceLast, varianceSpeed);

    float maxDeltaLength = ColorKernel / 50.0;

    if (any(greaterThan(deltaColor, vec3(maxDeltaLength))))
    {
        float maxDeltaColor = max(deltaColor.r, max(deltaColor.g, deltaColor.b));
        deltaColor *= maxDeltaLength / maxDeltaColor;
    }

    vec3 finalColor = colorLast.rgb + deltaColor;

    imageStore(BlendedResult, causticsPixelPos, vec4(finalColor, weightedVariance));




}