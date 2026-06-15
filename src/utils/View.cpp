#include "View.h"


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, View* view, float deltaTime);

int fbw=SCR_WIDTH;
int fbh=SCR_HEIGHT;

View::View()
{
    
    initWindow();
                
    
}

int View::initWindow()
{


    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }    

   
    glfwGetFramebufferSize(window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);  

 
    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    
    return 0;

    
    
}

void View::initSetting()
{
    std::cout<<"begin initSetting"<<std::endl;
    scene=new Scene();

    std::cout<<"Compile Ray Trace Shader"<<std::endl;
    rayTraceShader=new Shader("../src/shaders/vScreen.vs", "../src/shaders/rayTracing.fs");
    std::cout<<"Compile gbuffer Shader"<<std::endl;
    gBufferShader=new Shader("../src/shaders/gBuffer.vs", "../src/shaders/gBuffer.fs");
    std::cout<<"Compile shadow Map Shader"<<std::endl;
    shadowMapShader=new Shader("../src/shaders/gBuffer.vs", "../src/shaders/shadowMap.fs");
    std::cout<<"Compile filter Shader"<<std::endl;
    filterShader=new Shader("../src/shaders/vScreen.vs", "../src/shaders/denoise.fs");
    std::cout<<"Compile temporal Shader"<<std::endl;
    temporalShader=new Shader("../src/shaders/vScreen.vs", "../src/shaders/temporalDenoise.fs");
    
    std::cout<<"end Scene Shader"<<std::endl;

    //Photon Shader
    std::cout<<"Compile updateRayDensity Shader"<<std::endl;
    updateRayDensityShader=new Shader("../src/shaders/updateRayDensity.cs");
    std::cout<<"Compile mipmap0 Shader"<<std::endl;
    mipMap0Shader=new Shader("../src/shaders/rayCountMipMap0.cs");
    std::cout<<"Compile mipmap Shader"<<std::endl;
    mipMapShader=new Shader("../src/shaders/rayCountMipMap.cs");
    std::cout<<"Compile emit photon Shader"<<std::endl;
    emitPhotonShader=new Shader("../src/shaders/emitPhotons.cs");
    std::cout<<"Compile scatter Shader"<<std::endl;
    scatterPhotonShader=new Shader("../src/shaders/photonScatter.vs", "../src/shaders/photonScatter.fs");
    std::cout<<"Compile caustics blended result Shader"<<std::endl;
    blendCausticsShader=new Shader("../src/shaders/photonFilter.cs");

    std::cout<<"end photon shader"<<std::endl;

    mainCamera=new Camera(glm::vec3(0.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -35.0f);
    //mainCamera=new Camera(glm::vec3(0.0f, 0.0f, 3.0f));
    lightCamera=new Camera(scene->mainLight->pos, glm::vec3(0.0f, 0.0f, -1.0f), 0.0f, -90.0f,90.0f);

    screen=new Quad();

    photonMap=new PhotonMap();

    std::cout<<"end camera screen PhotonMap"<<std::endl;

    initGBuffer();
    initShadowBuffer();
    initRayTraceBuffer(gRayTr_fbo, gTrColor);
    initRayTraceBuffer(gFilter_fbo, gFilterColor);
    initFilterMap();

    initRayTraceBuffer(gCaustic_fbo, gCaustics); //for photon Map;

    std::cout<<"end init Buffer"<<std::endl;
    

}

void View::initGBuffer()
{
     //init g-Buffer and textures
        glGenFramebuffers(1, &gBuffer_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer_fbo);

       

        // position buffer
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
      
        // normal buffer
        glGenTextures(1, &gNormal);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
             
         //Generate Depth buffer
        glGenTextures(1,&gDepth);
        glBindTexture(GL_TEXTURE_2D, gDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RG, GL_FLOAT, nullptr);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gDepth, 0);

        // color buffer
        glGenTextures(1, &gColor);
        glBindTexture(GL_TEXTURE_2D, gColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gColor, 0);

        //ID Buffer
        glGenTextures(1,&gID);
        glBindTexture(GL_TEXTURE_2D, gID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gID, 0);

        // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
        unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4};
        glDrawBuffers(5, attachments);

        

        // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
        unsigned int rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
      
        // finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void View::bindingRayTraceShader()
{
    rayTraceShader->use();
    rayTraceShader->setInt("objNum", scene->glasses.size());
    rayTraceShader->setFloat("width",(float)SCR_WIDTH);
    rayTraceShader->setFloat("height", (float)SCR_HEIGHT);


    //uniform vec3 lightPos;
    //uniform vec3 lightColor;

    rayTraceShader->setInt("verticesTex",0);
    rayTraceShader->setVec2("verticesTexSize",glm::vec2((float)scene->verticesTex_width));

    rayTraceShader->setInt("gPosition",1);
    rayTraceShader->setInt("gNormal",2);
    rayTraceShader->setInt("gDepth",3);
    rayTraceShader->setInt("gColor",4);
    rayTraceShader->setInt("shadowMap",5);
    rayTraceShader->setInt("causticsMap",6);
   

    rayTraceShader->setVec3("mainCamera.position", mainCamera->Position);
    rayTraceShader->setVec3("mainCamera.front", mainCamera->Front);
    rayTraceShader->setVec3("mainCamera.right", mainCamera->Right);
    rayTraceShader->setVec3("mainCamera.up", mainCamera->Up);
    rayTraceShader->setFloat("mainCamera.near", mainCamera->Near);
    rayTraceShader->setFloat("mainCamera.far", mainCamera->Far);
    rayTraceShader->setFloat("mainCamera.zoom", mainCamera->Zoom);

    rayTraceShader->setVec3("light.position", scene->mainLight->pos);
    rayTraceShader->setVec3("light.color", scene->mainLight->color);
    rayTraceShader->setFloat("light.intensity", scene->mainLight->intensity);

    rayTraceShader->setMat4("shadowTrans", lightCamera->GetProjMatrix(SCR_WIDTH,SCR_HEIGHT, false)*lightCamera->GetViewMatrix());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene->verticesTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gColor);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, gShadow);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, photonMap->blendedResult);


}

void View::renderGBuffer()
{
    gBufferShader->use();

    //初始设置
    glm::mat4 view=mainCamera->GetViewMatrix();
    glm::mat4 projection=mainCamera->GetProjMatrix(SCR_WIDTH,SCR_HEIGHT, false);
    gBufferShader->setMat4("view",view);
    gBufferShader->setMat4("projection",projection);

    gBufferShader->setInt("frameID",frameID);

    glEnable(GL_DEPTH_TEST);
	glDepthMask( GL_TRUE );
    //glBindFramebuffer(GL_FRAMEBUFFER,0);
    glBindFramebuffer(GL_FRAMEBUFFER,gBuffer_fbo);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    //绘制
    scene->drawPlanes(gBufferShader);
    scene->drawGlasses(gBufferShader);

    glBindVertexArray(0);
    
}

void View::initShadowBuffer()
{
     //init g-Buffer and textures
        glGenFramebuffers(1, &gShadow_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, gShadow_fbo);
            
         //Generate Depth buffer
        glGenTextures(1,&gShadow);
        glBindTexture(GL_TEXTURE_2D, gShadow);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RG, GL_FLOAT, nullptr);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gShadow, 0);

        // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
        unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, attachments);

        

        // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
        unsigned int rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
      
        // finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void View::renderShadowMap()
{
    shadowMapShader->use();

    shadowMapShader->setMat4("view",lightCamera->GetViewMatrix());
    shadowMapShader->setMat4("projection",lightCamera->GetProjMatrix(SCR_WIDTH,SCR_HEIGHT, false));

    glEnable(GL_DEPTH_TEST);
	glDepthMask( GL_TRUE );
    //glBindFramebuffer(GL_FRAMEBUFFER,0);
    glBindFramebuffer(GL_FRAMEBUFFER,gShadow_fbo);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    //draw scene
    scene->drawPlanes(shadowMapShader);
    scene->drawGlasses(shadowMapShader);
    glBindVertexArray(0);
    
}

void View::initRayTraceBuffer(GLuint &fbo, GLuint &tex)
{
        //Ray tracing result FBO
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            
        
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

        // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
        unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, attachments);

        

        // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
        unsigned int rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
      
        // finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void View::rayTracing(Quad *screen)
{
    glBindFramebuffer(GL_FRAMEBUFFER,gRayTr_fbo);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glClearColor(0.5f, 0.4f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //draw scene
        
    //binding info to rayTraceShader
    std::cout<<"begin bind ray trace shader"<<std::endl;
    bindingRayTraceShader();
	rayTraceShader->use();    
	glBindVertexArray(screen->VAO);
    std::cout<<"begin ray trace"<<std::endl;
	glDrawArrays(GL_TRIANGLES,0,6);
    glFinish();
    std::cout<<"end ray trace"<<std::endl;

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

}

void View::initFilterMap()
{   
    //For color
    glGenTextures(1, &lastTrColor);
    glBindTexture(GL_TEXTURE_2D, lastTrColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    

    //For ID
    glGenTextures(1,&lastID);
    glBindTexture(GL_TEXTURE_2D, lastID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   

    // last normal buffer
    glGenTextures(1, &lastNormal);
    glBindTexture(GL_TEXTURE_2D, lastNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
             
    //last Depth buffer
    glGenTextures(1,&lastDepth);
    glBindTexture(GL_TEXTURE_2D, lastDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RG, GL_FLOAT, nullptr);
        
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    //last caustics
    glGenTextures(1, &lastCaustics);
    glBindTexture(GL_TEXTURE_2D, lastCaustics);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    //clear to zero
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lastTrColor, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lastNormal, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, lastDepth, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, lastCaustics, 0);

    
    GLenum bufs[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3
    };
    glDrawBuffers(4, bufs);

    // 清 float / UNORM 纹理
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

 


}
void View::filter(Quad *screen)
{
   
    glBindFramebuffer(GL_FRAMEBUFFER,gFilter_fbo);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glClearColor(0.5f, 0.4f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    filterShader->use();

    filterShader->setFloat("width",(float)SCR_WIDTH);
    filterShader->setFloat("height", (float)SCR_HEIGHT);

    filterShader->setInt("gPosition",0);
    filterShader->setInt("gNormal",1);
    filterShader->setInt("gTrColor",2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gTrColor);

    glBindVertexArray(screen->VAO);
	glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	
    

}

void View::temporalFilter(Quad *screen, int frameID)
{
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glClearColor(0.5f, 0.4f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    temporalShader->use();
    temporalShader->setFloat("width",(float)SCR_WIDTH);
    temporalShader->setFloat("height", (float)SCR_HEIGHT);
    temporalShader->setInt("frameID",frameID);

    temporalShader->setMat4("lastViewProj", lastViewProj);

    temporalShader->setInt("gPosition",0);
    temporalShader->setInt("gTrColor",1);
    temporalShader->setInt("lastTrColor",2);
    temporalShader->setInt("currIDMap",3);
    temporalShader->setInt("lastIDMap",4);
    temporalShader->setInt("causticsMap",5);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gFilterColor);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, lastTrColor);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gID);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, lastID);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, photonMap->blendedResult);

    glBindVertexArray(screen->VAO);
	glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
}
void View::initRayDensity()
{
    //init ray 

}

void View::updateRayDensity()
{
    //

}

int View::display()
{
    std::cout<<"begin Display"<<std::endl;

    
    initSetting();
    //Quad for rayTracing
    //Quad screen;

    std::cout<<"end initSetting"<<std::endl;


    frameID=0;

    float lastFrame = 0.0f;


    while(!glfwWindowShouldClose(window))
    {
        std::cout<<"begin render loop"<<std::endl;

        frameID++;

        float currentFrame = (float)glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

         //input
        processInput(window, this, deltaTime);

        //update lights array
        scene->lights[0] = *scene->mainLight;

        //update light camera 

        //[photon mapping] pass 0: update light ID

        //[Todo] add lights
        //[Current] only one light needs PM

         //pass 1: G-Buffer
        renderGBuffer();

        

        //[photon mapping] pass 1: create mipmap task id for rays
        photonMap->generateMipMap0(*mipMap0Shader, photonMap->gRayDensityTexA);
        photonMap->generateMipMap(*mipMapShader, photonMap->gRayDensityTexA);

        //[photon mapping] pass 2: emit photons
        photonMap->updateLightInfoSSBO(scene->lights, scene->lights.size());
        photonMap->resetPixelInfoSSBO();
        photonMap->resetPhotonCounter();
        photonMap->emitPhotons(*emitPhotonShader, photonMap->gRayDensityTexA, scene->verticesTex, glm::vec2((float)scene->verticesTex_width), gDepth, *mainCamera, scene->glasses.size());

        //[photon mapping] pass 3: update ray density
        photonMap->updateRayDensity(*updateRayDensityShader, photonMap->gRayDensityTexA, photonMap->gRayDensityTexB);

         GLuint tempRayDensity;
         tempRayDensity=photonMap->gRayDensityTexA;
         photonMap->gRayDensityTexA=photonMap->gRayDensityTexB;
         photonMap->gRayDensityTexB=tempRayDensity;


        //[photon mapping] pass 4: scattering photons
        photonMap->getPhotonCount();
        photonMap->scatterPhotons(gCaustic_fbo, *scatterPhotonShader, *mainCamera, gDepth, gNormal, gColor);

        //[photon mapping] pass 5: filter
        photonMap->filter(*blendCausticsShader, gDepth, lastDepth, gNormal, lastNormal, gCaustics, lastCaustics, *mainCamera, lastViewProj, lastProj);

        //pass 2: Shadow Map
        // renderShadowMap();


        //pass 3: Ray Tracing
        rayTracing(screen);

        //pass 4: Filtering
        filter(screen);

        //pass 5: Temporal filtering
        temporalFilter(screen, frameID);


        //update tex
       glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

        glBindTexture(GL_TEXTURE_2D, lastTrColor);
        glCopyTexSubImage2D(
            GL_TEXTURE_2D,
            0,              // mip level
            0, 0,           // offset in texture
            0, 0,           // x, y in screen (default framebuffer)
            SCR_WIDTH, SCR_HEIGHT   // region size
        );

        glCopyImageSubData(
            gID, GL_TEXTURE_2D, 0, 0, 0, 0,  
            lastID, GL_TEXTURE_2D, 0, 0, 0, 0,  
            SCR_WIDTH, SCR_HEIGHT, 1                   
        );

        glCopyImageSubData(
            gNormal, GL_TEXTURE_2D, 0, 0, 0, 0,  
            lastNormal, GL_TEXTURE_2D, 0, 0, 0, 0,  
            SCR_WIDTH, SCR_HEIGHT, 1                   
        );

        glCopyImageSubData(
            gDepth, GL_TEXTURE_2D, 0, 0, 0, 0,  
            lastDepth, GL_TEXTURE_2D, 0, 0, 0, 0,  
            SCR_WIDTH, SCR_HEIGHT, 1                   
        );

        glCopyImageSubData(
            gCaustics, GL_TEXTURE_2D, 0, 0, 0, 0,  
            lastCaustics, GL_TEXTURE_2D, 0, 0, 0, 0,  
            SCR_WIDTH, SCR_HEIGHT, 1                   
        );
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

        //output to picture
        // std::string filename = "videoImages/" + std::to_string(frameID) + ".png";
        // Output::SaveDefaultFBOToImage(filename.c_str(), SCR_WIDTH, SCR_HEIGHT);
               

        // 
        glfwPollEvents();
        glfwSwapBuffers(window);

        //更新last
        lastProj = mainCamera->GetProjMatrix(SCR_WIDTH,SCR_HEIGHT,false);
        lastViewProj= lastProj*mainCamera->GetViewMatrix();
        //update scene and camera
        scene->updateScene();
    }


    glfwTerminate();
    

   
    
    return 0;
    
} 



// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

//for keyboard
void processInput(GLFWwindow* window, View* view, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 0.1f * deltaTime;

    glm::vec3& lightPos = view->scene->mainLight->pos;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        lightPos.y += speed;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        lightPos.y -= speed;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        lightPos.x -= speed;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        lightPos.x += speed;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        lightPos.z -= speed;

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        lightPos.z += speed;
}