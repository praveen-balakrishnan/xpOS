#include "Graphics.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "Text.h"

namespace xpOS::GraphicsLib
{

void Font::draw_text(std::string text, int ix, int iy, int lineH, uint32_t colour, const Framebuffer& buffer)
{
    float scale = stbtt_ScaleForPixelHeight(&m_info, lineH);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&m_info, &ascent, &descent, &lineGap);

    ascent = roundf(ascent * scale);
    descent = roundf(descent * scale);

    static constexpr int b_w = 1024;
    static constexpr int b_h = 128;

    unsigned char* bitmap = reinterpret_cast<unsigned char*>(calloc(b_w * b_h, sizeof(unsigned char)));

    auto* cstr = text.c_str();

    int i = 0;
    int cx = 0;
    int cy = 0;

    for (int i = 0; i < strlen(cstr); i++) {
        if (cstr[i] == '\n') {
            cx = 0;
            cy += lineH;
            continue;
        }

        if (cy + lineH >= b_h)
            break;
        
        int ax;
        int lsb;
        stbtt_GetCodepointHMetrics(&m_info, cstr[i], &ax, &lsb);

        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&m_info, cstr[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

        int y = ascent + c_y1 + cy;

        if (cx + roundf(ax * scale) >= b_w)
            return;

        int byteOffset = cx + roundf(lsb * scale) + (y * b_w);
        stbtt_MakeCodepointBitmap(&m_info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, b_w, scale, scale, cstr[i]);

        cx += roundf(ax * scale);

        int kern;
        kern = stbtt_GetCodepointKernAdvance(&m_info, cstr[i], cstr[i + 1]);
        cx += roundf(kern * scale);
    }

    int endX = ix + b_w;
    if (endX > buffer.width)
        endX = buffer.width;
    
    int endY = iy + b_h;
    if (endY > buffer.height)
        endY = buffer.height;

    for (int r = 0; r < endY - iy; r++) {
        for (int c = 0; c < endX - ix; c++) {
            buffer.data[(r + iy) * buffer.width + (c + ix)] =
                blend(bitmap[r * b_w + c], (bitmap[r * b_w + c] << 24) | (colour & 0x00FFFFFF));
        }
    }

    free(bitmap);
}

}
