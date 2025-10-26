#version 430 core

layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform float width;
uniform float height;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gTrColor;


const int kernelRadius=16;


float m_sigmaPlane = 0.1f;
float m_sigmaColor = 0.6f;
float m_sigmaNormal = 0.1f;
float m_sigmaCoord = 32.0f;


vec4 Filter()
{
    vec3 result = vec3(0.0);
    float total_weight = 0.0;
    int x=int(TexCoords.x*width);
    int y=int(TexCoords.y*height);

    int x_left = max(0, x - kernelRadius);
    int x_right = min(int(width) - 1, x + kernelRadius);
    int y_bottom = max(0, y - kernelRadius);
    int y_top = min(int(height) - 1, y + kernelRadius);

    vec3 filtering_position = texture(gPosition,TexCoords).xyz;
    vec3 filtering_normal = texture(gNormal,TexCoords).xyz;
    vec3 filtering_colour = texture(gTrColor,TexCoords).xyz;

    float weight = 0.0;

    for (int kernal_x = x_left; kernal_x <= x_right; ++kernal_x) {
        for (int kernal_y = y_bottom; kernal_y <= y_top; ++kernal_y) {

            vec2 kernel_coords=vec2(float(kernal_x),float(kernal_y))/vec2(width,height);
            vec3 kernal_position = texture(gPosition,kernel_coords).xyz;
            vec3 kernal_colour = texture(gTrColor,kernel_coords).xyz;
            vec3 kernal_normal = texture(gNormal,kernel_coords).xyz;

            float dist = pow(distance(filtering_position, kernal_position), 2.0);
            float D_position = dist / (2.f * pow(m_sigmaCoord, 2));
            float D_colour = pow(distance(filtering_colour,kernal_colour),2.0) / (2.0 * pow(m_sigmaColor, 2));
            float D_normal = pow(acos(dot(filtering_normal, kernal_normal)), 2.0) / (2.0 * pow(m_sigmaNormal, 2));
                    
            float D_plane = 0.f;
            if (dist > 0.f) {
                D_plane = pow(dot(filtering_normal, normalize(kernal_position - filtering_position)),2.0) / (2.f * pow(m_sigmaPlane, 2));
            }

            float kernal_weight = exp(-D_position - D_colour - D_normal - D_plane);
            weight += kernal_weight;
            result += kernal_colour * kernal_weight;
        }
    }
    if (weight>0.0001) {
        return vec4(result / weight,1.0);
    } 
    else {
        return vec4(filtering_colour,1.0);
    }
}

void main()
{
    
    FragColor = Filter();

}