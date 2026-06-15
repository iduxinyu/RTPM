#include "Scene.h"

Scene::Scene()
{
    initScene();

    std::cout<<"end initScene"<<std::endl;
}

bool Scene::initScene()
{
    //init box
    glm::mat4 model=glm::mat4(1.0f);
    model=glm::translate(model,glm::vec3(1.5f,0.0f,0.0f));
    model=glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
    model=glm::scale(model,glm::vec3(10.0f));
    Plane* left=new Plane(glm::vec3(0.63f, 0.065f, 0.05f),model,0.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(-1.5f,0.0f,0.0f));
    model=glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 0.0, 1.0));
    model=glm::scale(model,glm::vec3(10.0f));
    Plane* right=new Plane(glm::vec3(0.14f, 0.45f, 0.091f),model,1.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,1.5f,0.0f));
    model=glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));
    model=glm::scale(model,glm::vec3(10.0f));
    Plane* top=new Plane(glm::vec3(0.3,0.3,0.8), model,2.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-1.5f,0.0f));
    //model=glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0, 0.0, 1.0));
    model=glm::scale(model,glm::vec3(10.0f));
    Plane* bottom=new Plane(glm::vec3(0.725f, 0.71f, 0.68f), model,3.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,0.0f,-5.0f));
    model=glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
    model=glm::scale(model,glm::vec3(10.0f));
    Plane* back=new Plane(glm::vec3(0.725f, 0.71f, 0.68f), model, 4.0);

    planes.push_back(left);
    planes.push_back(right);
    planes.push_back(top);
    planes.push_back(bottom);
    planes.push_back(back);

    std::cout<<"end planes"<<std::endl;

    //cube 
    float v[] =  {
            //---position------   --Normal--           --texCoord-  
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, 
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,

            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,

             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
        };

    //diffuse cube
    // model=glm::translate(glm::mat4(1.0f),glm::vec3(-0.5f,-0.7f,-2.5f));
    // //model =glm::rotate(model, glm::radians(80.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    // model=glm::scale(model,glm::vec3(1.0f));
    // Glass* g1=new Glass(v,glm::vec3(0.8f,0.0f,0.9f),REFRACT,model,8,36, planes.size()+0.0);

    // //glass cube
    // model=glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-1.3f,-2.5f));
    // //model =glm::rotate(model, glm::radians(-50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // model =glm::rotate(model, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // model=glm::scale(model,glm::vec3(0.5f));
    // Glass* g2=new Glass(v,glm::vec3(0.8f,0.8f,0.0f),REFRACT,model,8,36, planes.size()+1.0);


    //glass matrix scene
    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.7f,-1.3f,-2.5f));
    //model =glm::rotate(model, glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model=glm::scale(model,glm::vec3(0.3f,0.5f,0.1f));
    Glass* g2=new Glass(v,glm::vec3(0.8f,0.2f,0.9f),REFRACT,model,8,36, planes.size()+0.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(-0.7f,-1.3f,-2.5f));
    //model =glm::rotate(model, glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model=glm::scale(model,glm::vec3(0.3f,0.5f,0.1f));
    Glass* g3=new Glass(v,glm::vec3(1.0f,1.0f,1.0f),REFRACT,model,8,36, planes.size()+1.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-1.3f,-1.8f));
    model =glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model=glm::scale(model,glm::vec3(0.3f,0.5f,0.1f));
    Glass* g4=new Glass(v,glm::vec3(0.9f,0.9f,0.2f),REFRACT,model,8,36, planes.size()+2.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-1.3f,-3.2f));
    model =glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model=glm::scale(model,glm::vec3(0.3f,0.5f,0.1f));
    Glass* g5=new Glass(v,glm::vec3(0.2f,0.2f,1.0f),REFRACT,model,8,36, planes.size()+3.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.5f,-1.3f,-3.0f));
    model =glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model=glm::scale(model,glm::vec3(0.3f,0.5f,0.1f));
    Glass* g6=new Glass(v,glm::vec3(0.2f,0.9f,0.5f),REFRACT,model,8,36, planes.size()+4.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(-0.5f,-1.3f,-3.0f));
    model =glm::rotate(model, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model=glm::scale(model,glm::vec3(0.3f,0.5f,0.1f));
    Glass* g7=new Glass(v,glm::vec3(0.6f,0.3f,0.9f),REFRACT,model,8,36, planes.size()+5.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(0.5f,-1.3f,-2.0f));
    model =glm::rotate(model, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model=glm::scale(model,glm::vec3(0.3f,0.5f,0.1f));
    Glass* g8=new Glass(v,glm::vec3(0.7f,0.9f,0.3f),REFRACT,model,8,36, planes.size()+6.0);

    model=glm::translate(glm::mat4(1.0f),glm::vec3(-0.5f,-1.3f,-2.0f));
    model =glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model=glm::scale(model,glm::vec3(0.3f,0.5f,0.1f));
    Glass* g9=new Glass(v,glm::vec3(0.9f,0.6f,0.5f),REFRACT,model,8,36, planes.size()+7.0);
    
    //glasses.push_back(g1);
    glasses.push_back(g2);
    glasses.push_back(g3);
    glasses.push_back(g4);
    glasses.push_back(g5);
    glasses.push_back(g6);
    glasses.push_back(g7);
    glasses.push_back(g8);
    glasses.push_back(g9);
  
  
    std::cout<<"end Glasses"<<std::endl;

    //Pack the glasses' vertex data and material attributes into a texture, 
    //including world-space position, normal, UV coordinates, object color, and material type (refraction, specular reflection, or diffuse reflection).
    initVerticesMap(glasses);

    std::cout<<"end init vertices Map"<<std::endl;

    //light source
    mainLight=new Light(glm::vec3(-1.2f,0.0f,-2.5f), glm::vec3(0.0,-1.0f,0.0), glm::vec3(1.0), 15.0f);
    lights.push_back(*mainLight);

    std::cout<<"end lights"<<std::endl;

    return true;
    

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


    //fill data into tex, and use cuda later
    texData.assign(verticesTex_width * verticesTex_width, glm::vec4(0.0f));

    for (int obj = 0; obj < glasses.size(); ++obj) {

        int offset = obj * verticesTex_width;

        //1 0 pixel：meta info
        texData[offset + 0] = glm::vec4(
            (float)glasses[obj]->verticesNum, // r: vertices number
            (float)glasses[obj]->indicesNum,  // g: inidces number
            (float)glasses[obj]->type,        // b: material type                          
            0.0f                              // a: 
        );

        //2 1st pixel: color
         texData[offset + 1] = glm::vec4(
            glasses[obj]->color,                         
            0.0f                              
        );

        //3 2~5pixel model matrix
       
        texData[offset + 2] = glasses[obj]->model[0];
        texData[offset + 3] = glasses[obj]->model[1];
        texData[offset + 4] = glasses[obj]->model[2];
        texData[offset + 5] = glasses[obj]->model[3];

        //4 vertex data (pos.xyz, u) (normal.xyz, v)
        for (size_t i = 0; i < glasses[obj]->indicesNum; ++i) {
           
            texData[offset + 6 + i*2  + 0] = glm::vec4(glasses[obj]->vertices[8*i],glasses[obj]->vertices[8*i+1],glasses[obj]->vertices[8*i+2], glasses[obj]->vertices[8*i+6]);
            texData[offset + 6 + i*2  + 1] = glm::vec4(glasses[obj]->vertices[8*i+3],glasses[obj]->vertices[8*i+4],glasses[obj]->vertices[8*i+5], glasses[obj]->vertices[8*i+7]);
           
        }

       
    }

    //bind into tex
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

    glBindTexture(GL_TEXTURE_2D, verticesTex);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0, 0, 0,
        verticesTex_width, verticesTex_width,
        GL_RGBA, GL_FLOAT,
        texData.data()
    );


}


bool Scene::initBVH()
{
    
}

void Scene::updateScene()
{
    //rotate glasses 1
    // glm::mat4 model=glasses[0]->model;
    // model=glm::rotate(model, glm::radians(5.0f), glm::vec3(0.0, 1.0, 0.0));
    // glasses[0]->model=model;

    // updateVerticesMap(0);


    //update light 
    // float angle = glfwGetTime()*0.01; 

    // glm::vec3 center = glm::vec3(0.0f, 0.0f, -2.5f);

    // float radius = 1.2f;

    // glm::vec3 pos;

    // pos.x = center.x + cos(angle) * radius;
    // pos.y = center.y;
    // pos.z = center.z + sin(angle) * radius;
    // mainLight->pos = pos;
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