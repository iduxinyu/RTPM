#version 430 core
layout (location = 0) out vec2 gDepth;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;


void main()
{    
    gDepth.x=gl_FragCoord.z;
    //gDepth.xy=Normal.xy;
 
}