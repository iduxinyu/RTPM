cmake_minimum_required(VERSION 3.6)

project(Opengl)

set(CMAKE_CXX_STANDARD 17)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out)



# =========================
# Windows
# =========================
if(WIN32)

    include_directories(include/WIN)

    # 本地库目录
    link_directories(${PROJECT_SOURCE_DIR}/lib)

    # 查找 glfw
    find_library(GLFW_LIB
        NAMES glfw3
        PATHS ${PROJECT_SOURCE_DIR}/lib
    )

    # 查找 assimp
    find_library(ASSIMP_LIB
        NAMES assimp libassimp
        PATHS ${PROJECT_SOURCE_DIR}/lib
    )

    if(NOT GLFW_LIB)
        message(FATAL_ERROR "Cannot find glfw3 library")
    endif()

    if(NOT ASSIMP_LIB)
        message(FATAL_ERROR "Cannot find assimp library")
    endif()

endif()

# =========================
# Linux / Unix
# =========================
if(UNIX)


    # find_package(OpenGL REQUIRED)
    # find_package(glfw3 REQUIRED)
    # find_package(assimp REQUIRED)

    link_directories(${PROJECT_SOURCE_DIR}/lib)
   

    include_directories(${PROJECT_SOURCE_DIR}/include/UNIX/glfw/include
        ${PROJECT_SOURCE_DIR}/include/UNIX/glm
        ${PROJECT_SOURCE_DIR}/include/UNIX/imgui
        ${PROJECT_SOURCE_DIR}/include/UNIX/spdlog
        ${PROJECT_SOURCE_DIR}/include/UNIX/imgui/backends
        ${PROJECT_SOURCE_DIR}/include/UNIX/glad/include)

endif()

# =========================
# Source Files
# =========================
file(GLOB_RECURSE SRC_FILE
    src/*.c
    src/*.cpp
)

# =========================
# Executable
# =========================
add_executable(main ${SRC_FILE})

# =========================
# Link Libraries
# =========================
if(WIN32)

    target_link_libraries(main
        ${GLFW_LIB}
        ${ASSIMP_LIB}
    )

elseif(UNIX)
     target_link_libraries(main PRIVATE /usr/lib/x86_64-linux-gnu/libglfw.so.3)

endif()