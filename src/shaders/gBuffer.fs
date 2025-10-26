#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec2 gDepth;
layout (location = 3) out vec4 gColor;
layout (location = 4) out uvec4 gID;




in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform vec3 pureColor;

uniform int mat;
uniform int objID;


void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;
    // also store the per-fragment normals into the gbuffer
    gNormal.rgb = normalize(Normal).rgb;
    //gNormal.a = texture(texture_specular1, TexCoords).b;    
    gNormal.a = mat; //mat
   
    gDepth.x=gl_FragCoord.z;

    gColor=vec4(pureColor,1.0);

    gID=uvec4(objID,0,0,1);
  
}