/**
    Copyright 2023-2025 Praveen Balakrishnan

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    xpOS v1.0
*/

#include "Graphics.h"

namespace BitmapGraphics
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

/*void fill_rect(const RenderBuffer& buffer, Rect fillRect, const Rect& clipRect, const Viewport& viewport, uint32_t colour)
{
    fillRect = viewport.intersect(fillRect);
    if (fillRect.width == 0 || fillRect.height == 0)
        return;
    fillRect = clipRect.intersect(fillRect);
    if (fillRect.width == 0 || fillRect.height == 0)
        return;
    
    auto vc = ViewportCoord::from_world(fillRect.coord, viewport);
    for (int r = 0; r < fillRect.height; r++) {
        for (int c = 0; c < fillRect.width; c++) {
            int idx = (vc.x + c) + (vc.y + r) * viewport.width;
            buffer.buffer[idx] = BitmapGraphics::blend(buffer.buffer[idx], colour);
            //buffer.buffer[idx] = colour;//buffer.buffer[idx];
        }
    }
}*/


}
