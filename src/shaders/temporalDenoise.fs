#version 430 core

in vec2 TexCoords;
out vec4 FragColor;

uniform float width;
uniform float height;

uniform int frameID;

uniform mat4 lastViewProj;

uniform sampler2D gPosition;
uniform sampler2D gTrColor;
uniform sampler2D lastTrColor;
uniform sampler2D lastIDMap;
uniform sampler2D currIDMap;

const int kernelRadius=3;

float m_colorBoxK = 1.0f;
float m_alpha = 0.8f;

///////////////////Temporal Filter/////////////////////
vec2 reprojection()
{
    vec3 position=texture(gPosition, TexCoords).xyz;
    vec4 lastFramePos=lastViewProj*vec4(position,1.0);
    lastFramePos/=lastFramePos.w;

    if(lastFramePos.x <=-1 || lastFramePos.x >= 1 || lastFramePos.y <= -1 || lastFramePos.y >= 1)
        return vec2(-1.0);
    
    lastFramePos.xy=lastFramePos.xy*0.5+0.5;

    //check id
    float lastId=texture(lastIDMap,lastFramePos.xy).y;
    float currentId=texture(currIDMap, TexCoords).y;
    if(lastId!=currentId)
        return vec2(-1.0);

    return lastFramePos.xy;
}

vec3 TemporalAccumulation(vec3 currentColor, vec2 lastTexCoord)
{
    if(lastTexCoord.x<=0.0 || lastTexCoord.y<=0.0)
        return currentColor;

    //sigma and mu of currentFrame
    int x=int(TexCoords.x*width);
    int y=int(TexCoords.y*height);

    int x_start = max(0, x - kernelRadius);
    int x_end = min(int(width) - 1, x + kernelRadius);
    int y_start = max(0, y - kernelRadius);
    int y_end = min(int(height) - 1, y + kernelRadius);

    vec3 mu=vec3(0.0);
    vec3 sigma=vec3(0.0);

    vec3 filtering_colour = texture(gTrColor,TexCoords).xyz;

    for (int m = x_start; m <= x_end; m++) {
        for (int n = y_start; n <= y_end; n++) {
            vec2 kernel_coords=vec2(float(m),float(n))/vec2(width,height);
            vec3 kernal_colour = texture(gTrColor,kernel_coords).xyz;
            mu += kernal_colour;
            sigma += (filtering_colour - kernal_colour)*(filtering_colour - kernal_colour);
        }
    }
    float count =float(kernelRadius)*2.0+1.0;
    count*=count;
    mu/=count;
    sigma=sigma/count;
    sigma=sqrt(sigma);

    vec3 lastColor=texture(lastTrColor, lastTexCoord).xyz;
    lastColor=clamp(lastColor, mu - sigma * m_colorBoxK, mu + sigma * m_colorBoxK);

    return mix(lastColor, filtering_colour, m_alpha);
}



void main()
{
    vec4 color=texture(gTrColor,TexCoords);
    if(frameID==1)
    {
        FragColor=color;
    }
    else
    {
        vec2 lastTexCoord=reprojection();
        FragColor = vec4(TemporalAccumulation(color.xyz,lastTexCoord),1.0);

    }
    //FragColor=color;
        
}