#include "PhotonMap.h"


PhotonMap::PhotonMap()
{
    //init param
    gCoarseWidth=512;
    gCoarseHeight=512;




    //init

    std::cout<<"init Raydensity"<<std::endl;
    initRayDensityTextures(5.0);
    std::cout<<"init ParamsUBO"<<std::endl;
    initParamsUBO();
    std::cout<<"init PixelInfoSSBO"<<std::endl;
    initPixelInfoSSBO();
    std::cout<<"init QuadTree"<<std::endl;
    initQuadTreeSSBO();
    std::cout<<"init mipmapOffset"<<std::endl;
    initMipmapOffsetSSBO();
    std::cout<<"init LightInfo"<<std::endl;
    initLightInfoSSBO(3); //max 3 light sources
    std::cout<<"init Photons"<<std::endl;
    initPhotonsSSBO();
    std::cout<<"init CounterAtomic"<<std::endl;
    initPhotonCounterAtomic();
    std::cout<<"init VarianceTex"<<std::endl;
    initVarianceTexture();
    std::cout<<"init scatter Photon"<<std::endl;
    initScatterPhoton();
    std::cout<<"init blended result"<<std::endl;
    initBlendedResult();
    std::cout<<"init PhotonMap OK"<<std::endl;

    
}


/////////////////////////////////////
/// ray density
/////////////////////////////////////
void PhotonMap::initRayDensityTextures(float initialDensity)
{

    glGenTextures(1, &gRayDensityTexA);
    glGenTextures(1, &gRayDensityTexB);

    // ---------- Texture A ----------
    glBindTexture(GL_TEXTURE_2D, gRayDensityTexA);

    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F,
                   gCoarseWidth, gCoarseHeight);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // ---------- Texture B ----------
    glBindTexture(GL_TEXTURE_2D, gRayDensityTexB);

    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F,
                   gCoarseWidth, gCoarseHeight);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // ---------- Clear ----------
    std::vector<float> data(gCoarseWidth * gCoarseHeight, initialDensity);

    glBindTexture(GL_TEXTURE_2D, gRayDensityTexA);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                0, 0,
                gCoarseWidth, gCoarseHeight,
                GL_RED, GL_FLOAT,
                data.data());

    glBindTexture(GL_TEXTURE_2D, gRayDensityTexB);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                0, 0,
                gCoarseWidth, gCoarseHeight,
                GL_RED, GL_FLOAT,
                data.data());

    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT |
                    GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}


void PhotonMap::initParamsUBO()
{
    glGenBuffers(1, &paramsUBO);

    glBindBuffer(GL_UNIFORM_BUFFER, paramsUBO);

    glBufferData(
        GL_UNIFORM_BUFFER,
        sizeof(RayDensityParams),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    setParamsUBO(glm::ivec2(gCoarseWidth, gCoarseHeight), 5.0f, 1.0f, 0.1f, 9, 0.1f);
}
 
void PhotonMap::setParamsUBO(glm::ivec2 coarseDim, float minPhotonPixelSize, float varianceGain, float smoothWeight, int maxTaskPerPixel, float updateSpeed)
{
    RayDensityParams params{};
    params.CoarseDim = coarseDim;
    params.MinPhotonPixelSize = minPhotonPixelSize;
    params.VarianceGain = varianceGain;
    params.SmoothWeight = smoothWeight;
    params.MaxTaskPerPixel = maxTaskPerPixel;
    params.UpdateSpeed = updateSpeed;

    glBindBuffer(GL_UNIFORM_BUFFER, paramsUBO);

    glBufferSubData(
        GL_UNIFORM_BUFFER,
        0,
        sizeof(RayDensityParams),
        &params
    );

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void PhotonMap::initPixelInfoSSBO()
{
    glGenBuffers(1, &pixelInfoSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pixelInfoSSBO);

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        sizeof(PixelInfo) * gCoarseWidth * gCoarseHeight,
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}

void PhotonMap::resetPixelInfoSSBO()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pixelInfoSSBO);

    std::vector<PixelInfo> zero(gCoarseWidth * gCoarseHeight);

    glBufferSubData(
        GL_SHADER_STORAGE_BUFFER,
        0,
        zero.size() * sizeof(PixelInfo),
        zero.data()
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


void PhotonMap::updateRayDensity(Shader shader, GLuint rayDensitySourceTex, GLuint rayDensityTargetTex)
{
    shader.use();
    std::cout<<"bind tex"<<std::endl;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rayDensitySourceTex);

    glBindImageTexture(1, rayDensityTargetTex, 0, GL_FALSE, 0,
                       GL_WRITE_ONLY, GL_R32F);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, pixelInfoSSBO); 
    glBindBufferBase(GL_UNIFORM_BUFFER, 3, paramsUBO);

    GLuint gx = (gCoarseWidth  + 15) / 16;
    GLuint gy = (gCoarseHeight + 15) / 16;
    std::cout<<"UPDATE ray den"<<std::endl;
    glDispatchCompute(gx, gy, 1);

    std::cout<<"end rayden"<<std::endl;

    // ✔ 只需要一次
    glMemoryBarrier(
        GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
        GL_TEXTURE_FETCH_BARRIER_BIT |
        GL_SHADER_STORAGE_BARRIER_BIT
    );
    std::cout<<"end barrier"<<std::endl;

}

//////////////////////////////////////
/// light quad tree
//////////////////////////////////////

void PhotonMap::initQuadTreeSSBO()
{
    totalNodeCount = 0;

    int w = gCoarseWidth/2;
    int h = gCoarseHeight/2;

    while (w > 0 && h > 0)
    {
        totalNodeCount += w * h;
        w /= 2;
        h /= 2;

        std::cout<<"totalNodeCount: "<<totalNodeCount<<std::endl;
    }

    

    glGenBuffers(1, &quadTreeSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, quadTreeSSBO);

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        totalNodeCount * sizeof(glm::uvec4),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void PhotonMap::initMipmapOffsetSSBO()
{
    // init offset
    computeMipmapOffset();

    glGenBuffers(1, &mipOffsetSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mipOffsetSSBO);

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        mipOffset.size() * sizeof(uint32_t),
        mipOffset.data(),
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void PhotonMap::computeMipmapOffset()  //level=0 : root
{
    mipDepth = (int)floor(log2(std::max(gCoarseWidth/2, gCoarseHeight/2)));

    mipOffset.resize(mipDepth + 1);  //level=0 ~ mipDepth

    int offset = 0;

    for(int level = 0; level <= mipDepth; level++)
    {
        mipOffset[level] = offset;

        offset += 1 << (2 * level); //next level offset
    }

    std::cout<<"mipDepth: "<<mipDepth<<std::endl;
    std::cout<<"width: "<<gCoarseWidth/2<<std::endl;
}


void PhotonMap::generateMipMap0(Shader shader, GLuint rayDensityTex)
{
    shader.use();
    //binding SSBO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,quadTreeSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,mipOffsetSSBO);
  
    //binding rayDensitytex
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, rayDensityTex);
   
    //binding uniform
    shader.setInt("MipLevel",mipDepth);

    //generatemipmap
    glDispatchCompute((gCoarseWidth/2+7)/8,(gCoarseHeight/2+7)/8,1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

}

void PhotonMap::generateMipMap(Shader shader, GLuint rayDensityTex)
{
    shader.use();
    //binding SSBO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,quadTreeSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,mipOffsetSSBO);

    //binding uniform

    for(int level = mipDepth-1 ; level >=0; level--)
    {
        glm::ivec2 dim = glm::ivec2(1 << level);

        shader.setInt("MipLevel", level);

        glDispatchCompute(
            (dim.x + 7) / 8,
            (dim.y + 7) / 8,
            1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

}



////////////////////////////////////////////
///   emit Photons
////////////////////////////////////////////
void PhotonMap::initLightInfoSSBO(int MaxLightNum)
{
    glGenBuffers(1, &lightsInfoSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightsInfoSSBO);

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        MaxLightNum * sizeof(LightInfo),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void PhotonMap::updateLightInfoSSBO(std::vector<Light> lights, int lightNum)
{
    std::vector<LightInfo> lightsInfo;
    for(auto light:lights)
    {
        LightInfo lightInfo;

        lightInfo.position=glm::vec4(light.pos,sqrt(light.intensity / 0.02)); 
        lightInfo.direction=glm::vec4(light.dir, (float)light.type);
        lightInfo.color=glm::vec4(light.color,light.intensity);
        lightInfo.spotPro=light.spotPro;
        lightInfo.rectPro=light.rectPro;

        lightsInfo.push_back(lightInfo);

    }

      
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightsInfoSSBO);

    glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                    0,
                    lightsInfo.size() * sizeof(LightInfo),
                    lightsInfo.data());
}

void PhotonMap::initPhotonsSSBO()
{
    glGenBuffers(1, &photonsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, photonsSSBO);

    int maxPhotons = 1000000;

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        maxPhotons * sizeof(Photon),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void PhotonMap::initPhotonCounterAtomic()
{
    
    glGenBuffers(1, &photonCounterAtomic);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, photonCounterAtomic);

    glBufferData(GL_ATOMIC_COUNTER_BUFFER,
             sizeof(int),
             nullptr,
             GL_DYNAMIC_DRAW);
}

void PhotonMap::resetPhotonCounter()
{
    int zero = 0;

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, photonCounterAtomic);

    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER,
                0,
                sizeof(int),
                &zero);
}

void PhotonMap::initVarianceTexture()
{
    glGenTextures(1, &VarianceTexture);

    glBindTexture(GL_TEXTURE_2D, VarianceTexture);

    glTexImage2D(GL_TEXTURE_2D,
             0,
             GL_R32F,              // 单通道 float（推荐）
             SCR_WIDTH,
             SCR_HEIGHT,
             0,
             GL_RED,
             GL_FLOAT,
             nullptr);             // ❗不传数据（只分配）

    // 采样参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //clear to zero
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER,
                       GL_COLOR_ATTACHMENT0,
                       GL_TEXTURE_2D,
                       VarianceTexture,
                       0);

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
}

void PhotonMap::emitPhotons(Shader shader, GLuint currentRayDensityTexture, GLuint verticesTex, glm::vec2 verticesTexSize, GLuint depthTex, Camera camera, int objNum)
{
    shader.use();
   
    //bind SSBO
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,quadTreeSSBO);   //in
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,mipOffsetSSBO);  //in
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,lightsInfoSSBO); //in
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER,4,photonsSSBO);    //out
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER,5,pixelInfoSSBO);  //out

   

   //bind Atomic Buffer
   glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 6, photonCounterAtomic); //inout

  
   //bind texture
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, currentRayDensityTexture);

   glActiveTexture(GL_TEXTURE7);
   glBindTexture(GL_TEXTURE_2D, blendedResult);

   glActiveTexture(GL_TEXTURE8);
   glBindTexture(GL_TEXTURE_2D, depthTex);

   glActiveTexture(GL_TEXTURE9);
   glBindTexture(GL_TEXTURE_2D, verticesTex);


    //bind uniform
    shader.setInt("mipDepth", mipDepth);  //quad tree mip depth
    shader.setVec2i("LightMapSize", glm::ivec2(gCoarseWidth, gCoarseHeight));
    shader.setVec2i("ViewportDim", glm::ivec2(SCR_WIDTH, SCR_HEIGHT));
    shader.setFloat("Intensity", intensity);
    shader.setFloat("SplatSize", splatSize);

    
    
    shader.setFloat("MidCullColorThreshold", midCullThreshold);
    shader.setFloat("MaxBounceDistance", maxBounceDistance);
    shader.setFloat("MaxScreenRadius", maxScreenRadius);

    shader.setVec2("verticesTexSize", verticesTexSize);
    shader.setInt("objNum", objNum);

    glm::mat4 view=camera.GetViewMatrix();
    glm::mat4 projection=camera.GetProjMatrix(SCR_WIDTH,SCR_HEIGHT,false);
    shader.setMat4("ViewProjectionMatrix", projection*view);

    //begin emit photon pass
    glDispatchCompute(
        (gCoarseWidth*10 + 7) / 8,
        (gCoarseHeight*10 + 7) / 8,
        1
    );

    // 保证 SSBO 写入完成
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
                    GL_ATOMIC_COUNTER_BARRIER_BIT);

    glFinish();

}

/////////////////////////////////////////
/// scatterPhotons
/////////////////////////////////////////

void PhotonMap::getPhotonCount()
{
    
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, photonCounterAtomic);
    glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(int), &photonCount);
}

void PhotonMap::initScatterPhoton()
{
    glGenVertexArrays(1, &photonVAO);
    glBindVertexArray(photonVAO);
}


// scatter photons
void PhotonMap::scatterPhotons(GLuint fbo, Shader shader, Camera camera, GLuint depthTex, GLuint normalTex, GLuint albedoTex)
{
    shader.use();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    //state
    glDisable(GL_DEPTH_TEST);   // ❗ 必须
    glDepthMask(GL_FALSE);      // 不写 depth

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    //binding ssbo
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,photonsSSBO);  
    

    //binding texture

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalTex);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, albedoTex);

    shader.setMat4("ViewMatrix", camera.GetViewMatrix());
    shader.setMat4("ProjectionMatrix", camera.GetProjMatrix(SCR_WIDTH,SCR_HEIGHT,false));
    shader.setVec3("CameraPosition",camera.Position);
    shader.setVec2("ScreenDim",glm::vec2(SCR_WIDTH,SCR_HEIGHT));

    shader.setFloat("SplatSize", splatSize);
    shader.setFloat("MaxAnisotropy", MaxAnisotropy);
    shader.setInt("PhotonMode", 0);  // anisotropic =1     unaniso=0

    shader.setFloat("ZTolerance",0.01);
    shader.setFloat("MinRoughness", 0.04);

    glBindVertexArray(photonVAO);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, photonCount);

     std::cout<<"photon Count: "<<photonCount<<std::endl;

    // 恢复 depth
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    // 恢复 blend
    glDisable(GL_BLEND);

    //（可选）恢复  默认 blend func
    glBlendFunc(GL_ONE, GL_ZERO);



}


/////////////////////////////////////////
/// filter
/////////////////////////////////////////

void PhotonMap::initBlendedResult()
{
    //init blended result
    glGenTextures(1, &blendedResult);
    glBindTexture(GL_TEXTURE_2D, blendedResult);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,             
        SCR_WIDTH,
        SCR_HEIGHT,
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr
    );

   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   
}


void PhotonMap::filter(Shader shader, GLuint DepthTextureThis, GLuint DepthTextureLast, GLuint NormalTextureThis, GLuint NormalTextureLast, GLuint CausticsTextureThis, GLuint CausticsTextureLast, Camera camera, glm::mat4 lastViewProj, glm::mat4 lastProj)
{
    shader.use();

 

    //bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, DepthTextureThis);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, DepthTextureLast);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, NormalTextureThis);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, NormalTextureLast);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, CausticsTextureThis);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, CausticsTextureLast);

    

    //output
    glBindImageTexture(
        6,                  // binding = 6
        blendedResult,      // texture id
        0,                  // mip level
        GL_FALSE,           
        0,                  
        GL_WRITE_ONLY,      
        GL_RGBA32F
    );

   

    //bind uniform
    shader.setMat4("ReprojectionMatrix", lastViewProj*glm::inverse(camera.GetProjMatrix(SCR_WIDTH, SCR_HEIGHT, false)*camera.GetViewMatrix()));
    shader.setMat4("InverseProjectionMatrixLast", glm::inverse(lastProj));

    shader.setInt("Enable", 1);
    shader.setVec2i("CausticsDim", glm::ivec2(SCR_WIDTH,SCR_HEIGHT));
    shader.setVec2i("GBufferDim", glm::ivec2(SCR_WIDTH,SCR_HEIGHT));

    shader.setFloat("BlendWeight", 0.9f);
    shader.setFloat("NormalKernel", 0.1f);
    shader.setFloat("DepthKernel", 1.0f);
    shader.setFloat("ColorKernel", 1.0f);

   

    //begin compute
    glDispatchCompute(
        (SCR_WIDTH  + 15) / 16,
        (SCR_HEIGHT + 15) / 16,
        1
    );
    std::cout<<"begin barrier"<<std::endl;

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
   


}
