#pragma once

#include<iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
     
#include <vector>

#include "Object.h"

#define SCR_WIDTH 512  
#define SCR_HEIGHT 512


class Scene
{
public:
    
    std::vector<Plane*> planes;
    std::vector<Glass*> glasses;

    Light *mainLight;
    std::vector<Light> lights;

    BVHNode* bvh;

    //pacakage vertex info into shader for ray tracing
    GLuint verticesTex;
    int verticesTex_width;
    std::vector<glm::vec4> texData; 

    int MaxLightNum;
    

public:
    Scene();

    void initSetting(); //init setting
    bool initScene(); //set scene and package into tex
    void initVerticesMap(std::vector<Glass*> glasses);
    void updateVerticesMap(int index);
    bool initBVH();
    bool updateBVH();
    void rayTracing();
    void updateScene();
    

    void drawPlanes(Shader* shader);
    void drawGlasses(Shader* shader);

};