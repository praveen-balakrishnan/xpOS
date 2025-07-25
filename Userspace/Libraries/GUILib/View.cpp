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

#include "View.h"

namespace xpOS::GUILib
{

LayoutNode FrameView::computeLayout(const Size& suggestion)
{
        Size clampedSize;
        clampedSize.width = width.has_value() ? *width : clamp(suggestion.width, minWidth, maxWidth);
        clampedSize.height = height.has_value() ? *height : clamp(suggestion.height, minHeight, maxHeight);

        LayoutNode childLayout = child->computeLayout(clampedSize);
        Size childSize = childLayout.size;

        LayoutNode node(shared_from_this());
        node.size.width =  width.has_value() ? *width : clamp(childSize.width, minWidth, maxWidth);
        node.size.height = height.has_value() ? *height : clamp(childSize.height, minHeight, maxHeight);

        childLayout.origin.x = (node.size.width - childLayout.size.width) / 2;
        childLayout.origin.y = (node.size.height - childLayout.size.height) / 2;
        node.children.push_back(childLayout);
        
        return node;
    }

std::shared_ptr<FrameView> View::frame(
    std::optional<int> width,
    std::optional<int> height,
    std::optional<int> minWidth,
    std::optional<int> minHeight,
    std::optional<int> maxWidth,
    std::optional<int> maxHeight
)
{
    return std::make_shared<class FrameView>(
        shared_from_this(),
        width,
        height,
        minWidth,
        minHeight,
        maxWidth,
        maxHeight
    );
}

}
