#version 430 core

struct Photon
{
    vec4 posW;      // xyz + padding
    vec4 color;
    vec4 dPdx;
    vec4 dPdy;
    vec4 dir;
};

layout(std430, binding = 0) buffer PhotonBufferSSBO
{
    Photon photons[];
};


uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform vec3 CameraPosition;
uniform vec2 ScreenDim;

uniform float SplatSize;
uniform float MaxAnisotropy;

uniform int PhotonMode; 
// 0 = Anisotropic
// 1 = Isotropic

out VS_OUT
{
    vec3 normal;
    vec2 texcoord;
    vec3 lightDir;
    vec4 color;
    vec2 screenPos;
} vs_out;

const vec2 quad[6] = vec2[6](
    vec2(-1,-1),
    vec2( 1,-1),
    vec2( 1, 1),
    vec2(-1,-1),
    vec2( 1, 1),
    vec2(-1, 1)
);

vec3 ScaleVector(vec3 vec, vec3 axis, vec2 factor)
{
    axis = normalize(axis);
    float proj = dot(vec, axis);
    vec *= factor.y;
    vec += axis * proj * (factor.x - factor.y);
    return vec;
}

void main()
{
    uint id = gl_InstanceID;
    Photon p = photons[id];

    vec3 Position  = p.posW.xyz;
    vec3 dPdx      = p.dPdx.xyz;
    vec3 dPdy      = p.dPdy.xyz;
    vec3 Direction = p.dir.xyz;
    vec3 Color     = p.color.xyz;

    vec3 normal = normalize(cross(dPdx, dPdy));

    vec3 tangent;
    vec3 bitangent;

    // =========================
    // Photon Mode
    // =========================
    if (PhotonMode == 0) // Anisotropic
    {
        tangent   = dPdx;
        bitangent = dPdy;

        if (dot(tangent, bitangent) < 0.0)
            bitangent *= -1.0;

        vec3 areaVector   = cross(tangent, bitangent);
        vec3 sideVector   = tangent + bitangent;
        float area = length(areaVector);
        float side = length(sideVector);
        float height = area / side;
        float aniso  = side / height;

        if (aniso > MaxAnisotropy)
        {
            tangent = ScaleVector(tangent, sideVector, vec2(MaxAnisotropy / aniso, 1));
			bitangent = ScaleVector(bitangent, sideVector, vec2(MaxAnisotropy / aniso, 1));
        }
    }
    else // Isotropic
    {
        tangent   = normalize(vec3(normal.y, -normal.x, 0.0));
        bitangent = cross(tangent, normal);

        float radius = max(length(dPdx), length(dPdy));

        tangent   *= radius;
        bitangent *= radius;
    }

    tangent   *= SplatSize;
    bitangent *= SplatSize;

    // screen footprint（用于优化）
    vec3 areaVec = cross(tangent, bitangent);
    vec3 viewDir = Position - CameraPosition;
    float dist   = length(viewDir);
    float screenArea = length(areaVec) / (dist * dist) * length(ScreenDim);

    //world Position of the quad
    vec2 v = quad[gl_VertexID];  //determine which vertex on quad
    vec3 localPoint = tangent * v.x + bitangent * v.y;
    vec4 worldPos = vec4(localPoint + Position, 1.0);

    gl_Position = ProjectionMatrix * ViewMatrix * worldPos;
    vs_out.screenPos = (gl_Position.xy / gl_Position.w) * 0.5 + 0.5;

   

    vs_out.normal = normal;
    vs_out.texcoord = v;
    vs_out.lightDir = Direction;  
    vs_out.color = vec4(Color, screenArea);


   
   
}