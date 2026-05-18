#pragma once


#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>   
#include "Shader.h"


enum Axis{xAxis=0,yAxis=1,zAxis=2};
enum MATERIAL{DIFFUSE=0,SPECULAR=1,REFRACT=2};
enum LightType{LIGHT_POINT=0, LIGHT_SPOT = 1, LIGHT_DIRECTIONAL = 2, LIGHT_RECT = 3 };

struct BVHNode
{
    int glassIndex;
    glm::vec3 min;
    glm::vec3 max;

    BVHNode* left;
    BVHNode* right;


};

class BVH
{

};

class Quad{

public:
    unsigned int VAO;
    unsigned int VBO;

public:
    Quad()
    {
        
        float vertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
        };

        glGenVertexArrays(1, &VAO);
	    glBindVertexArray(VAO);//绑定VAO
	    
	    glGenBuffers(1, &VBO);//生成ID，并给予缓冲
	    glBindBuffer(GL_ARRAY_BUFFER, VBO);//把VBO绑到GL_ARRAY_BUFFER上代表VBO是一个顶点缓冲
	    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);//把vertices放到顶点缓存中

        //链接顶点属性，告诉GPU如何理解传入的顶点数据
	    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); //连接到GL_ARRAY_BUFFER绑定的VBO
	    glEnableVertexAttribArray(0);

	    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float))); //连接到GL_ARRAY_BUFFER绑定的VBO
	    glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        

    }



    ~Quad()
    {
        ///释放资源
	    glDeleteVertexArrays(1, &VAO);
	    glDeleteBuffers(1, &VBO);
    }


};


class Plane
{
public:
    unsigned int VAO;
    unsigned int VBO;

    glm::vec3 color;
    glm::mat4 model;

    int objID;

public:
    Plane(glm::vec3 color, glm::mat4 model, int id)
    {
        
        float vertices[] = {
            //---position------    ----Normal--------   --texCoord-
            -0.5f,  0.0f, -0.5f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
             0.5f,  0.0f, -0.5f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
             0.5f,  0.0f,  0.5f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.0f,  0.5f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.0f,  0.5f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.0f, -0.5f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f
            
        };

        glGenVertexArrays(1, &VAO);
	    glBindVertexArray(VAO);//绑定VAO
	    
	    glGenBuffers(1, &VBO);//生成ID，并给予缓冲
	    glBindBuffer(GL_ARRAY_BUFFER, VBO);//把VBO绑到GL_ARRAY_BUFFER上代表VBO是一个顶点缓冲
	    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);//把vertices放到顶点缓存中

        //链接顶点属性，告诉GPU如何理解传入的顶点数据
	    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); //连接到GL_ARRAY_BUFFER绑定的VBO
	    glEnableVertexAttribArray(0);

	    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float))); //连接到GL_ARRAY_BUFFER绑定的VBO
	    glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6*sizeof(float))); //连接到GL_ARRAY_BUFFER绑定的VBO
	    glEnableVertexAttribArray(2);
        glBindVertexArray(0);//解绑

        this->color=color;
        this->model=model;
        this->objID=id;

    }

    
    void Draw(Shader* shader)
    {
        shader->use();
		shader->setVec3("pureColor",color);
        shader->setMat4("model",model);
        shader->setInt("mat",DIFFUSE);
        shader->setInt("objID",objID);
       
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6); 
    }


    ~Plane()
    {
        ///释放资源
	    glDeleteVertexArrays(1, &VAO);
	    glDeleteBuffers(1, &VBO);
    }


};


class Glass
{
public:

    unsigned int VAO;
    unsigned int VBO;

    float* vertices;
    glm::vec3 color;
    MATERIAL type;
    glm::mat4 model;
    int verticesNum;//顶点数量
    int indicesNum; //顶点对应的索引数量

    int objID;


public:
    Glass(float* vertices, glm::vec3 color, MATERIAL type, glm::mat4 model, int verticesNum, int indicesNum, int id)
    {
        
       this->vertices=vertices;

        glGenVertexArrays(1, &VAO);
	    glBindVertexArray(VAO);//绑定VAO
	    
	    glGenBuffers(1, &VBO);//生成ID，并给予缓冲
	    glBindBuffer(GL_ARRAY_BUFFER, VBO);//把VBO绑到GL_ARRAY_BUFFER上代表VBO是一个顶点缓冲
	    glBufferData(GL_ARRAY_BUFFER, indicesNum*8*sizeof(float), vertices, GL_STATIC_DRAW);//把vertices放到顶点缓存中

        //链接顶点属性，告诉GPU如何理解传入的顶点数据
	    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); //连接到GL_ARRAY_BUFFER绑定的VBO
	    glEnableVertexAttribArray(0);

	    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float))); //连接到GL_ARRAY_BUFFER绑定的VBO
	    glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6*sizeof(float))); //连接到GL_ARRAY_BUFFER绑定的VBO
	    glEnableVertexAttribArray(2);
        glBindVertexArray(0);//解绑

        this->color=color;
        this->type=type;
        this->model=model;
        this->verticesNum=verticesNum;
        this->indicesNum=indicesNum;
        this->objID=id;

    }

    void Draw(Shader* shader)
    {
        shader->use();
		shader->setVec3("pureColor",color);
        shader->setMat4("model",model);
        shader->setInt("mat",type);
        shader->setInt("objID",objID);

       
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 36); 
    }


    ~Glass()
    {
        ///释放资源
	    glDeleteVertexArrays(1, &VAO);
	    glDeleteBuffers(1, &VBO);
    }

};

class Light
{
public:
    glm::vec3 pos;
    glm::vec3 dir;
    glm::vec3 color;
    float intensity;
    LightType type;
    glm::vec4 spotPro;
    glm::vec4 rectPro;

    Light(glm::vec3 pos, glm::vec3 dir, glm::vec3 color, float intensity, LightType type=LIGHT_POINT, float cosAngleSpot=0.0, glm::vec3 rectTangent=glm::vec3(0.0) )
    {
        this->color=color;
        this->pos=pos;
        this->dir=dir;
        this->intensity=intensity;
        this->type=type;
        this->spotPro.x=cosAngleSpot;
        this->rectPro=glm::vec4(rectTangent,0.0);
        
    }
};

struct alignas(16) RayDensityParams
{
    glm::ivec2 CoarseDim;        // 8
    float MinPhotonPixelSize;    // 4
    float VarianceGain;          // 4

    float SmoothWeight;          // 4
    int   MaxTaskPerPixel;       // 4
    float UpdateSpeed;           // 4
};
static_assert(sizeof(RayDensityParams) % 16 == 0);

struct LightInfo
{
    glm::vec4 position;   // 16B
    glm::vec4 direction;  // 16B
    glm::vec4 color;      // 16B
    glm::vec4 spotPro;    // 16B
    glm::vec4 rectPro;    // 16B
};

struct Photon
{
    glm::vec4 posW;
    glm::vec4 color;
    glm::vec4 dPdx;
    glm::vec4 dPdy;
    glm::vec4 dir;
};

struct PixelInfo
{
    int count;
    int screenArea;
    int screenAreaSq;
};