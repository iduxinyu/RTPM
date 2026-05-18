#pragma once

#include "stb_image_write.h"

#include <vector>
#include <GLFW/glfw3.h>

class Output
{

public:
    static void SaveDefaultFBOToImage(const char* filename, int width, int height)
    {
        std::vector<unsigned char> pixels(width * height * 4);

        // 读取默认 FBO（0 = screen buffer）
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glReadBuffer(GL_BACK);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);

        glReadPixels(
            0, 0,
            width, height,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels.data()
        );

        // flip Y
        std::vector<unsigned char> flipped(width * height * 4);

        for (int y = 0; y < height; ++y)
        {
            memcpy(
                &flipped[y * width * 4],
                &pixels[(height - 1 - y) * width * 4],
                width * 4
            );
        }

        stbi_write_png(filename, width, height, 4, flipped.data(), width * 4);
    }

};