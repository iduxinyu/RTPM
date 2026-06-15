#pragma once

#ifndef SHADER_H
#define SHADER_H


#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>



class Shader
{
public:

	unsigned int ID; 

public:

	Shader(const char* vertexPath, const char* fragmentPath);
	Shader(const char* computePath);


	void use();

	
	void setBool(const std::string &name, bool value) const;
	void setInt(const std::string &name, int value) const;
	void setFloat(const std::string &name, float value) const;
    void setVec2(const std::string& name, glm::vec2 value) const;
    void setVec2i(const std::string& name, glm::ivec2 value) const;
    void setVec3(const std::string& name, glm::vec3 value) const;
    void setVec4(const std::string& name, glm::vec4 value) const;
    void setMat2(const std::string& name, glm::mat2 value) const;
    void setMat3(const std::string& name, glm::mat3 value) const;
    void setMat4(const std::string &name, glm::mat4 value) const;


	~Shader();
};


#endif