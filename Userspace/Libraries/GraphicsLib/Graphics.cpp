#include "Graphics.h"

namespace xpOS::GraphicsLib
{

uint32_t blend(uint32_t bg, uint32_t fg)
{
    uint8_t fg_a = (fg >> 24) & 0xFF;
    
    if (fg_a == 0x00)
        return bg;
    if (fg_a == 0xFF)
        return fg;
    
    uint8_t fg_r = (fg >> 16) & 0xFF;
    uint8_t fg_g = (fg >> 8) & 0xFF;
    uint8_t fg_b = fg & 0xFF;

    uint8_t bg_r = (bg >> 16) & 0xFF;
    uint8_t bg_g = (bg >> 8) & 0xFF;
    uint8_t bg_b = bg & 0xFF;

    uint32_t alpha_factor = (fg_a << 8) + fg_a;
    uint32_t inv_alpha_factor = ((fg_a ^ 0xFF) << 8) + (fg_a ^ 0xFF);

    uint8_t out_r = (fg_r * alpha_factor + bg_r * inv_alpha_factor) >> 16;
    uint8_t out_g = (fg_g * alpha_factor + bg_g * inv_alpha_factor) >> 16;
    uint8_t out_b = (fg_b * alpha_factor + bg_b * inv_alpha_factor) >> 16;

    return (0xFF << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

}