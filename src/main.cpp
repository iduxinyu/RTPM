#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "utils/View.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "utils/stb_image_write.h"

int main()
{
    std::cout<<"Hello world"<<std::endl;

    
    View view=View();
    std::cout<<"end View"<<std::endl;

    view.display();
    std::cout<<"end Display"<<std::endl;


    return 0;
}