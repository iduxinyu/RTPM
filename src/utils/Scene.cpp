#include "Scene.h"

Scene::Scene()
{
    initScene();
}

bool Scene::initScene()
{
    //初始化最外围碰撞盒
    glm::mat4 model=glm::mat4(1.0f);
    model=glm::translate(model,glm::vec3(1.5f,0.0f,0.0f));
    model=glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
    model=glm::scale(model,glm::vec3(10.0f));
    Plane* left=new Plane(glm::vec3(0.8,0.3,0.3),model,0.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(-1.5f,0.0f,0.0f));
    model=glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 0.0, 1.0));
    model=glm::scale(model,glm::vec3(10.0f));
    Plane* right=new Plane(glm::vec3(0.3,0.8,0.3),model,1.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,1.5f,0.0f));
    model=glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));
    model=glm::scale(model,glm::vec3(10.0f));
    Plane* top=new Plane(glm::vec3(0.3,0.3,0.8), model,2.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-1.5f,0.0f));
    //model=glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0, 0.0, 1.0));
    model=glm::scale(model,glm::vec3(10.0f));
    Plane* bottom=new Plane(glm::vec3(0.75,0.75,0.75), model,3.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,0.0f,-5.0f));
    model=glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
    model=glm::scale(model,glm::vec3(10.0f));
    Plane* back=new Plane(glm::vec3(0.75,0.75,0.75), model, 4.0);

    planes.push_back(left);
    planes.push_back(right);
    planes.push_back(top);
    planes.push_back(bottom);
    planes.push_back(back);

    //创建一个glasses
    float v[] =  {
            //---position------   --Normal--           --texCoord-  
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, 
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
        };
    model=glm::translate(glm::mat4(1.0f),glm::vec3(-0.5f,-0.5f,-3.0f));
    model =glm::rotate(model, glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model=glm::scale(model,glm::vec3(1.0f));
    Glass* g1=new Glass(v,glm::vec3(1.0f),DIFFUSE,model,8,36, planes.size()+0.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.7f,0.7f,-1.5f));
    model =glm::rotate(model, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    //model =glm::rotate(model, glm::radians(-45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model=glm::scale(model,glm::vec3(1.0f));
    Glass* g2=new Glass(v,glm::vec3(1.0f),REFRACT,model,8,36, planes.size()+1.0);
    
    glasses.push_back(g1);
    glasses.push_back(g2);

    std::cout<<g2->type<<std::endl;

    //将glasses 的顶点信息和属性 打包进一个texture  顶点的世界位置，法线，uv 物体的颜色，是折射 镜面反射还是漫反射
    initVerticesMap(glasses);

    //生成光源
    mainLight=new Light(glm::vec3(0.0f,1.2f,-2.5f), glm::vec3(0.0,-1.0f,0.0), glm::vec3(1.0f), 3.0f);

}

void Scene::initVerticesMap(std::vector<Glass*> glasses)
{
    verticesTex_width=128;
    // RGBA32F texture
    glGenTextures(1, &verticesTex);
    glBindTexture(GL_TEXTURE_2D, verticesTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, verticesTex_width, verticesTex_width, 0, GL_RGBA, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    //填充数据到Tex  后期可以并行到cuda

    texData.assign(verticesTex_width * verticesTex_width, glm::vec4(0.0f));

    for (int obj = 0; obj < glasses.size(); ++obj) {

        int offset = obj * verticesTex_width;

        // 1 第0像素：meta 信息
        texData[offset + 0] = glm::vec4(
            (float)glasses[obj]->verticesNum, // r: 顶点数量
            (float)glasses[obj]->indicesNum,  // g: 索引数量
            (float)glasses[obj]->type,        // b: 材质类型                          
            0.0f                              // a: 
        );

        //2 第1像素 color
         texData[offset + 1] = glm::vec4(
            glasses[obj]->color,                         
            0.0f                              
        );

        //3 2~5像素 model matrix
       
        texData[offset + 2] = glasses[obj]->model[0];
        texData[offset + 3] = glasses[obj]->model[1];
        texData[offset + 4] = glasses[obj]->model[2];
        texData[offset + 5] = glasses[obj]->model[3];

        // 3 顶点数据
        for (size_t i = 0; i < glasses[obj]->indicesNum; ++i) {
           
            texData[offset + 6 + i*2  + 0] = glm::vec4(glasses[obj]->vertices[8*i],glasses[obj]->vertices[8*i+1],glasses[obj]->vertices[8*i+2], glasses[obj]->vertices[8*i+6]);
            texData[offset + 6 + i*2  + 1] = glm::vec4(glasses[obj]->vertices[8*i+3],glasses[obj]->vertices[8*i+4],glasses[obj]->vertices[8*i+5], glasses[obj]->vertices[8*i+7]);
           
        }

       
    }

    //绑到对应tex 上
    glBindTexture(GL_TEXTURE_2D, verticesTex);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0, 0, 0,
        verticesTex_width, verticesTex_width,
        GL_RGBA, GL_FLOAT,
        texData.data()
    );

}

void Scene::updateVerticesMap(int index)
{
    int offset=index*verticesTex_width;
    texData[offset + 2] = glasses[index]->model[0];
    texData[offset + 3] = glasses[index]->model[1];
    texData[offset + 4] = glasses[index]->model[2];
    texData[offset + 5] = glasses[index]->model[3];
}


bool Scene::initBVH()
{
    
}

void Scene::updateScene()
{
    //rotate glasses 1
    glm::mat4 model=glasses[0]->model;
    model=glm::rotate(model, glm::radians(10.0f), glm::vec3(0.0, 1.0, 0.0));
    glasses[0]->model=model;

    updateVerticesMap(0);
}

void Scene::drawPlanes(Shader* shader)
{
    for(int i=0;i<planes.size();i++)
        planes[i]->Draw(shader);
}

void Scene::drawGlasses(Shader* shader)
{
   
    for(int i=0;i<glasses.size();i++)
        glasses[i]->Draw(shader);
}