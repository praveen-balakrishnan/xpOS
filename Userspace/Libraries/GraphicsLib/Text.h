#ifndef GRAPHICSLIB_TEXT_H
#define GRAPHICSLIB_TEXT_H

#include <fstream>
#include <string>
#include <vector>
#include "Framebuffer.h"
#include "stb_truetype.h"

namespace xpOS::GraphicsLib
{

class Font
{
public:
    Font(std::string path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size))
            m_fontData = buffer;

        stbtt_InitFont(&m_info, reinterpret_cast<unsigned char*>(m_fontData.data()), 0);
    }

    void draw_text(std::string text, int x, int y, int lineH, uint32_t colour, const Framebuffer& buffer);

private:
    std::vector<char> m_fontData;
    stbtt_fontinfo m_info;
};

}

#endif