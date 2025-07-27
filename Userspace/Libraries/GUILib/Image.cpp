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

#include "GraphicsLib/stb_image.h"
#include "Image.h"

namespace xpOS::GUILib
{

ImageView::ImageView(std::string path)
{
    unsigned char *data = stbi_load(
        path.c_str(),
        &m_width,
        &m_height,
        nullptr,
        3
    );

    if (!data) {
        m_width = 0;
        m_height = 0;
        return;
    }

    m_pixelBuf = new uint32_t[m_width * m_height];

    for (int i = 0; i < m_width * m_height; i++)
        m_pixelBuf[i] = (0xFF << 24) | (data[i*3] << 16) | (data[i*3 + 1] << 8) | data[i*3+2];
}

LayoutNode ImageView::computeLayout(const Size& suggestion)
{
    LayoutNode node(shared_from_this());
    node.size = {m_width, m_height};
    return node;
}

void ImageView::draw(Point point, Size size, Context& context)
{
    
    const int destStartCol = std::max(0, point.x);
    const int destStartRow = std::max(0, point.y);
    const int destEndCol = std::min(context.size.width, point.x + size.width);
    const int destEndRow = std::min(context.size.width, point.y + size.height);
    const int destStride = context.size.width;

    const int srcStartCol = destStartCol - point.x;
    const int srcStartRow = destStartRow - point.y;
    const int srcStride = size.width;

    for (int r = 0; r < (destEndRow - destStartRow); r++) {
    const uint32_t* srcRowPtr = m_pixelBuf + (r + srcStartRow) * srcStride + srcStartCol;
    uint32_t* destRowPtr = context.framebuffer + (r + destStartRow) * destStride + destStartCol;

    for (int c = 0; c < (destEndCol - destStartCol); c++)
        destRowPtr[c] = srcRowPtr[c];
}

}

}
