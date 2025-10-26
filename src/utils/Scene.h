#pragma once

#include<iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
     
#include <vector>

#include "Object.h"

#define SCR_WIDTH 400  
#define SCR_HEIGHT 400


/*
class Planes 
{
public:
    Axis axis;
    glm::vec3 color;
    float distanceFromO;
    int opticalProp;

    Planes(enum Axis a,float d, glm::vec3 color )
    {
        axis =a;
        distanceFromO=d;
        this->color=color;
        opticalProp=0; //所有的墙壁默认diffuse
    }

};



class Glass
{

public:
    float* vertices;

    glm::vec3 color;
    MATERIAL type;
    glm::mat4 model;
    int verticesNum;//顶点数量
    int indicesNum; //顶点对应的索引数量
    

    Glass()
    {
        color=glm::vec3(1.0);
        type=DIFFUSE;

    }

};
*/
struct Ray
{

    glm::vec3 o;
    glm::vec3 d;
};

struct AABB
{
    glm::vec3 gMin;
    glm::vec3 gMax;

    bool intersect(Ray r)
    {

    }
    
    bool intersect(AABB b)
    {

    }
};

struct BVHNode
{
public:
    //包围盒
    AABB bbox;
    //子节点
    BVHNode* left;
    BVHNode* right;
    int index=-1; //对应 glasses 序列中的哪一个
};



class Scene
{
public:
    
    std::vector<Plane*> planes;
    std::vector<Glass*> glasses;

    Light *mainLight;

    BVHNode* bvh;

    //打包顶点信息用
    GLuint verticesTex;
    int verticesTex_width;
    std::vector<glm::vec4> texData; 
    

public:
    Scene();

    void initSetting(); //进行一些初始化设置
    bool initScene(); //设置场景，布置物体，并打包信息到Texture
    void initVerticesMap(std::vector<Glass*> glasses);
    void updateVerticesMap(int index);
    bool initBVH();
    bool updateBVH();
    void rayTracing();
    void updateScene();
    

    void drawPlanes(Shader* shader);
    void drawGlasses(Shader* shader);

};