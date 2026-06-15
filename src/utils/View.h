// Init Windows
// mouse Keyboard
//load Images
//load Shader

#pragma once

#include<iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Scene.h"
#include "Shader.h"
#include "Object.h"
#include "PhotonMap.h"
#include "Output.h"





class View 
{

public:
    GLFWwindow* window;
    Camera* mainCamera, *lightCamera;
    Scene* scene;
    Shader *rayTraceShader, *gBufferShader, *shadowMapShader, *filterShader, *temporalShader;

    Shader *updateRayDensityShader;
    Shader *mipMap0Shader, *mipMapShader;
    Shader *emitPhotonShader;
    Shader *scatterPhotonShader;
    Shader *blendCausticsShader;

    GLuint gBuffer_fbo, gPosition, gNormal, gDepth, gColor, gID;
    GLuint gShadow_fbo, gShadow;
    GLuint gRayTr_fbo, gTrColor;
    GLuint gFilter_fbo, gFilterColor;
    GLuint gCaustic_fbo, gCaustics;
    GLuint lastTrColor, lastID, lastDepth, lastNormal, lastCaustics;

    Quad *screen;

    int frameID;
    glm::mat4 lastProj;
    glm::mat4 lastViewProj;
    

    PhotonMap *photonMap;

    

public:
    View();
    int initWindow();
    void initSetting();
    int display();

    void initRayTraceBuffer(GLuint &fbo, GLuint &tex);
    void bindingRayTraceShader();
    void rayTracing(Quad *screen);

    void initGBuffer();
    void renderGBuffer();

    void initShadowBuffer();
    void renderShadowMap();

    void filter(Quad *screen);
    void temporalFilter(Quad *screen, int frameID);

    void initFilterMap();

    void initRayDensity();
    void updateRayDensity();


};