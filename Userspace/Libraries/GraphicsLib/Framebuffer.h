#ifndef GRAPHICSLIB_FRAMEBUFFER_H
#define GRAPHICSLIB_FRAMEBUFFER_H

#include <cstdint>

namespace xpOS::GraphicsLib
{

struct Framebuffer
{
    uint32_t* data;
    int width;
    int height;
};

}

#endif