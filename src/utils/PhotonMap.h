#pragma once

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "Camera.h"
#include "Shader.h"
#include "Object.h"

#define SCR_WIDTH 512
#define SCR_HEIGHT 512



class PhotonMap
{

public:

    int gCoarseWidth;
    int gCoarseHeight;
    
    //ray density
    GLuint pixelInfoSSBO;
    GLuint gRayDensityTexA;
    GLuint gRayDensityTexB;

    RayDensityParams params;
    GLuint paramsUBO;


    //mipmap
    GLuint quadTreeSSBO;
    GLuint mipOffsetSSBO;
    std::vector<uint32_t> mipOffset;
    int mipDepth;  
    int totalNodeCount;
    //emit photons
    GLuint lightsInfoSSBO; 
    GLuint photonsSSBO;   
    GLuint photonCounterAtomic;

    GLuint VarianceTexture; //varance between current frame and last frame

    float intensity = 3.0f;  //for energy scale
    float splatSize =5.0f;   //control size of  photon quad

    

    float midCullThreshold=0.0f; //cut low energy photon
    float maxBounceDistance=100.0f;
    float maxScreenRadius=100.0f;  //cut large footprint

    int photonCount;

    //scatter photon
    GLuint photonVAO;
    

    float MaxAnisotropy=10.0; //for deleting thin caustics

    //filter
    GLuint blendedResult;
    




public:
    PhotonMap();

    //update ray density
    void initRayDensityTextures(float initialDensity);
    void initParamsUBO();
    void setParamsUBO(glm::ivec2 coarseDim, float minPhotonPixelSize, float varianceGain, float smoothWeight, int maxTaskPerPixel, float updateSpeed);
    void initPixelInfoSSBO();
    void resetPixelInfoSSBO();
    void updateRayDensity(Shader shader, GLuint rayDensitySourceTex, GLuint rayDensityTargetTex);
    
    //build quad tree
    void initQuadTreeSSBO();
    void initMipmapOffsetSSBO();
    void computeMipmapOffset();
    void generateMipMap0(Shader shader, GLuint rayDensityTex);
    void generateMipMap(Shader shader, GLuint rayDensityTex);

    //emit photon
    void initLightInfoSSBO(int MaxLightNUm);
    void updateLightInfoSSBO(std::vector<Light> lights, int lightNum);
    void initPhotonsSSBO();
    void initPhotonCounterAtomic();
    void resetPhotonCounter();
    void initVarianceTexture();
    void emitPhotons(Shader shader, GLuint currentRayDensityTexture, GLuint verticesTex, glm::vec2 verticesTexSize, GLuint depthTex, Camera camera, int objNum);

    //scatter photons
    void initScatterPhoton();
    void getPhotonCount();
    void scatterPhotons(GLuint fbo, Shader shader, Camera camera, GLuint depthTex, GLuint normalTex, GLuint albedoTex);


    //filter caustics
    void initBlendedResult();
    void filter(Shader shader, GLuint DepthTextureThis, GLuint DepthTextureLast, GLuint NormalTextureThis, GLuint NormalTextureLast, GLuint CausticsTextureThis, GLuint CausticsTextureLast, Camera camera, glm::mat4 lastViewProj, glm::mat4 lastProj);



};