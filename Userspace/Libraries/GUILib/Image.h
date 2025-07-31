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

#ifndef XPOS_GUILIB_IMAGE_H
#define XPOS_GUILIB_IMAGE_H

#include "View.h"

namespace xpOS::GUILib
{

class ImageView : public View
{
public:
    ImageView(std::string path);

    LayoutNode computeLayout(const Size& suggestion) override;
    void draw(Point point, Size size, Context& context) override;

private:
    int m_width;
    int m_height;
    uint32_t* m_pixelBuf = nullptr;
};

std::shared_ptr<ImageView> Image(std::string path);

}

#endif
