cmake_minimum_required(VERSION 3.6)    #指示cmake最低版本
project(Opengl)                        #项目名称
set(CXX_STANDARD 17)                   #设置C++版本
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out)   #设置编译好的exe文件输出目录

include_directories(include)           #包含我们的引入的第三方头文件

link_directories(${PROJECT_SOURCE_DIR}/lib)  # 添加链接库的目录
find_library(GLFW_LIB NAMES glfw3 PATHS ${PROJECT_SOURCE_DIR}/lib)  # 在链接库中找到我们的glfw3链接库文件。
find_library(ASSIMP_LIB NAMES assimp libassimp PATHS ${PROJECT_SOURCE_DIR}/lib)  # 在链接库中找到我们的assimp链接库文件。

if(NOT GLFW_LIB)    #如果找不到glfw的话这里会报错。有可能你的glfw版本不会32位的。
    message(FATAL_ERROR "Cannot find glfw3 library")
endif()

if(NOT ASSIMP_LIB)    #如果找不到glfw的话这里会报错。有可能你的glfw版本不会32位的。
    message(FATAL_ERROR "Cannot find assimp library")
endif()

file(GLOB_RECURSE SRC_FILE src/*.c src/*.cpp utils/*.c utils/*.cpp)   #找到src中的所有c/cpp文件，然后指定名称位SRC_CILE，GLOB只是内置的一个参数，别在意。
add_executable(main ${SRC_FILE})        #开始生成可执行文件

target_link_libraries(main ${GLFW_LIB} ${ASSIMP_LIB}) 
