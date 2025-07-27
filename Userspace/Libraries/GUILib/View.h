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

#ifndef GUILIB_VIEW_H
#define GUILIB_VIEW_H

#include <memory>
#include <optional>
#include <vector>
#include "Context.h"

namespace xpOS::GUILib
{

class View;

struct LayoutNode
{
    Point origin = {0, 0};
    Size size = {0, 0};
    std::shared_ptr<View> view;
    std::vector<LayoutNode> children;
    LayoutNode(std::shared_ptr<View> viewPtr)
    : view(viewPtr)
    {}
};

class View : public std::enable_shared_from_this<View>
{
public:
    virtual ~View() = default;
    virtual LayoutNode computeLayout(const Size& suggestion) = 0;
    virtual void draw(Point origin, Size size, Context& context) = 0;
    virtual bool isFlexible() const
    {
        return false;
    }
    std::shared_ptr<FrameView> frame(
        std::optional<int> width = std::nullopt,
        std::optional<int> height = std::nullopt,
        std::optional<int> minWidth = std::nullopt,
        std::optional<int> minHeight = std::nullopt,
        std::optional<int> maxWidth = std::nullopt,
        std::optional<int> maxHeight = std::nullopt
    );
};

class FrameView : public View
{
public:
    std::shared_ptr<View> child;
    std::optional<int> width;
    std::optional<int> height;
    std::optional<int> minWidth;
    std::optional<int> minHeight;
    std::optional<int> maxWidth;
    std::optional<int> maxHeight;

    FrameView(
        std::shared_ptr<View> child,
        std::optional<int> width = std::nullopt,
        std::optional<int> height = std::nullopt,
        std::optional<int> minWidth = std::nullopt,
        std::optional<int> minHeight = std::nullopt,
        std::optional<int> maxWidth = std::nullopt,
        std::optional<int> maxHeight = std::nullopt
    )
    : child(child)
    , width(width)
    , height(height)
    , minWidth(minWidth)
    , minHeight(minHeight)
    , maxWidth(maxWidth)
    , maxHeight(maxHeight)
    {}

    LayoutNode computeLayout(const Size& suggestion) override;
    void draw(Point origin, Size size, Context& context) override {}

private:
    static int clamp(int value, std::optional<int> min, std::optional<int> max)
    {
        if (min.has_value())
            value = std::max(value, *min);
        if (max.has_value())
            value = std::min(value, *max);
        
        return value;
    }
};

class SpacerView : public View
{
public:
    SpacerView() = default;

    LayoutNode computeLayout(const Size& suggestion) override
    {
        LayoutNode node(shared_from_this());
        node.size = {0, 0};
        return node;
    }

    void draw(Point origin, Size size, Context& context) override {}

    bool isFlexible() const override
    {
        return true;
    }
};

}

#endif
