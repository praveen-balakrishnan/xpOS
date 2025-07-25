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

#include "Stack.h"

namespace xpOS::GUILib
{

LayoutNode HStackView::computeLayout(const Size& suggestion)
{
    int allocatedWidth = m_spacing * (m_childViews.size() - 1);
    int flexibleChildCount = 0;
    int childCount = m_childViews.size();
    int remainingFixed;
    int maxHeight = 0;
    std::vector<LayoutNode> layouts;
    layouts.reserve(m_childViews.size());

    for (auto& child : m_childViews) {
        if (child->isFlexible())
            flexibleChildCount++;
    }

    remainingFixed = childCount - flexibleChildCount;

    // TODO: Propose sizes to children in increasing order of flexibility.
    for (auto& child : m_childViews) {
        if (child->isFlexible()) {
            layouts.push_back(child->computeLayout({0, 0}));
        } else {
            int allocation = (suggestion.width - allocatedWidth) / remainingFixed;
            auto layout = child->computeLayout({allocation, suggestion.height});
            if (layout.size.height > maxHeight)
                maxHeight = layout.size.height;
            layouts.push_back(layout);
            allocatedWidth += layout.size.width;
            remainingFixed--;
        }
    }

    int flexWidth = 0;
    if (flexibleChildCount > 0 && suggestion.width > allocatedWidth) {
        flexWidth = (suggestion.width - allocatedWidth) / flexibleChildCount;
        allocatedWidth = suggestion.width;
    }

    LayoutNode node(shared_from_this());
    node.size.width = allocatedWidth;
    node.size.height = maxHeight;
    int x = 0;
    for (auto& layout : layouts) {
        if (layout.view->isFlexible()) {
            layout.size.width = flexWidth;
            layout.size.height = maxHeight;
        }

        layout.origin.x = x;
        layout.origin.y = (maxHeight - layout.size.height) / 2;
        x += layout.size.width;
        x += m_spacing;
    }
    node.children = std::move(layouts);

    return node;
}

LayoutNode VStackView::computeLayout(const Size& suggestion)
{
    int allocatedHeight = m_spacing * (m_childViews.size() - 1);
    int flexibleChildCount = 0;
    int remainingFixed;
    int childCount = m_childViews.size();
    int maxWidth = 0;
    std::vector<LayoutNode> layouts;
    layouts.reserve(m_childViews.size());

    for (auto& child : m_childViews) {
        if (child->isFlexible())
            flexibleChildCount++;
    }

    remainingFixed = childCount - flexibleChildCount;

    // TODO: Propose sizes to children in increasing order of flexibility.
    for (auto& child : m_childViews) {
        if (child->isFlexible()) {
            layouts.push_back(child->computeLayout({0, 0}));
        } else {
            int allocation = (suggestion.height - allocatedHeight) / remainingFixed;
            auto layout = child->computeLayout({suggestion.width, allocation});
            if (layout.size.width > maxWidth)
                maxWidth = layout.size.width;
            layouts.push_back(layout);
            allocatedHeight += layout.size.height;
            remainingFixed--;
        }
    }

    int flexHeight = 0;
    if (flexibleChildCount > 0 && suggestion.height > allocatedHeight) {
        flexHeight = (suggestion.height - allocatedHeight) / flexibleChildCount;
        allocatedHeight = suggestion.height;
    }

    LayoutNode node(shared_from_this());
    node.size.width = maxWidth;
    node.size.height = allocatedHeight;
    int y = 0;
    for (auto& layout : layouts) {
        if (layout.view->isFlexible()) {
            layout.size.width = maxWidth;
            layout.size.height = flexHeight;
        }

        layout.origin.y = y;
        layout.origin.x = (maxWidth - layout.size.width) / 2;
        y += layout.size.height;
        y += m_spacing;
    }
    node.children = std::move(layouts);

    return node;
}

}
