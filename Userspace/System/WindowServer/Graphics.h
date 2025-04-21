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

#ifndef WINDOWSERVER_GRAPHICS_H
#define WINDOWSERVER_GRAPHICS_H

#include <cstdint>
#include "Geometry.h"

struct RenderBuffer
{
    uint32_t* buffer;
    int width;
    int height;
};

namespace BitmapGraphics
{

/**
 * Performs alpha blending of a background and foreground colour.
*/
uint32_t blend(uint32_t background, uint32_t foreground);
//void unsafe_fill_rect(uint32_t* buffer, int bw, int bh, int x, int y, int w, int h, uint32_t colour);

/**
 * Fills a rectangular region with a specific colour, clipped to the viewport.
 * 
 * @tparam Blend whether alpha blending should be used.
 */
template <bool Blend = false>
void fill_rect(const RenderBuffer& buffer, const Rect& fillRect, const Viewport& viewport, uint32_t colour)
{
    auto vc = ViewportCoord::from_world(fillRect.coord, viewport);
    for (int r = 0; r < fillRect.height; r++) {
        for (int c = 0; c < fillRect.width; c++) {
            int idx = (vc.x + c) + (vc.y + r) * viewport.width;
            if constexpr (Blend)
                buffer.buffer[idx] = BitmapGraphics::blend(buffer.buffer[idx], colour);
            else
                buffer.buffer[idx] = colour;
        }
    }
}

/**
 * Fills a rectangular region with the contents of a buffer, clipped to the viewport.
 * 
 * @tparam Blend whether alpha blending should be used.
 * @param buffer buffer to write to.
 * @param src source buffer to read from.
 * @param rect the source buffer's world rect.
 * @param fillRect the world rect that should be filled.
 * @param viewport the viewport to which output will be clipped.
 */
template <bool Blend = false>
void buf_rect(const RenderBuffer& buffer, uint32_t* src, const Rect& rect, Rect fillRect, const Viewport& viewport)
{
    fillRect = rect.intersect(fillRect);
    if (rect.width == 0 || rect.height == 0)
        return;
    
    int startRow = fillRect.coord.y - rect.coord.y;
    int startCol = fillRect.coord.x - rect.coord.x;
    auto vc = ViewportCoord::from_world(fillRect.coord, viewport);
    for (int r = 0; r < fillRect.height; r++) {
        for (int c = 0; c < fillRect.width; c++) {
            int bufIdx = (vc.x + c) + (vc.y + r) * viewport.width;
            int srcIdx = (startCol + c) + (startRow + r) * rect.width;
            if constexpr (Blend)
                buffer.buffer[bufIdx] = BitmapGraphics::blend(buffer.buffer[bufIdx], src[srcIdx]);
            else
                buffer.buffer[bufIdx] = src[srcIdx];
        }
    }
}

}

#endif