// Init Windows
// mouse Keyboard
//load Images
//load Shader

#pragma once

#include<iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "Scene.h"
#include "Shader.h"
#include "Object.h"





class View 
{

private:
    GLFWwindow* window;
    Camera* mainCamera, *lightCamera;
    Scene* scene;
    Shader *rayTraceShader, *gBufferShader, *shadowMapShader, *filterShader, *temporalShader;

    GLuint gBuffer_fbo, gPosition, gNormal, gDepth, gColor, gID;
    GLuint gShadow_fbo, gShadow;
    GLuint gRayTr_fbo, gTrColor;
    GLuint gFilter_fbo, gFilterColor;
    GLuint lastTrColor, lastID;

    Quad *screen;

    int frameID;
    glm::mat4 lastViewProj;
    

public:
    View();
    int initWindow();
    void initSetting();
    int display();

    void initRayTraceBuffer(GLuint &fbo, GLuint &tex);
    void bindingRayTraceShader();

    void initGBuffer();
    void renderGBuffer();

    void initShadowBuffer();
    void renderShadowMap();

    void filter(Quad *screen);
    void temporalFilter(Quad *screen, int frameID);

    void initFilterMap();


private:
    //回调函数
    //void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    //void processInput(GLFWwindow *window);


};