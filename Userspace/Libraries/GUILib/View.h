#ifndef GUILIB_VIEW_H
#define GUILIB_VIEW_H

#include <memory>
#include <optional>
#include <vector>

struct Point
{
    int x;
    int y;
};

struct Size
{
    int width;
    int height;
};

class View;

struct LayoutNode
{
    Point origin;
    Size size;
    std::shared_ptr<View> view;
    std::vector<LayoutNode> children;
};

class View
{
public:
    virtual ~View() = default;
    virtual LayoutNode computeLayout(const Size& suggestion) = 0;
    virtual void draw(Point origin, Size size) = 0;
    virtual bool isFlexible() const
    {
        return false;
    }
};

class FrameView : public View
{
public:
    virtual ~FrameView() noexcept = default;
    std::shared_ptr<View> child;
    std::optional<int> width;
    std::optional<int> height;
    std::optional<int> minWidth;
    std::optional<int> minHeight;
    std::optional<int> maxWidth;
    std::optional<int> maxHeight;

    FrameView(
        std::shared_ptr<View> child,
        std::optional<int> width,
        std::optional<int> height,
        std::optional<int> minWidth,
        std::optional<int> minHeight,
        std::optional<int> maxWidth,
        std::optional<int> maxHeight
    )
    : child(child)
    , width(width)
    , height(height)
    , minWidth(minWidth)
    , minHeight(minHeight)
    , maxWidth(maxWidth)
    , maxHeight(maxHeight)
    {}

    LayoutNode computeLayout(const Size& suggestion) override
    {
        Size clampedSize;
        clampedSize.width = width.has_value() ? *width : clamp(suggestion.width, minWidth, maxWidth);
        clampedSize.height = height.has_value() ? *height : clamp(suggestion.height, minHeight, maxHeight);

        LayoutNode childLayout = child->computeLayout(clampedSize);
        Size childSize = childLayout.size;

        LayoutNode node;
        node.size.width =  width.has_value() ? *width : clamp(childSize.width, minWidth, maxWidth);
        node.size.height = height.has_value() ? *height : clamp(childSize.height, minHeight, maxHeight);

        childLayout.origin.x = (node.size.width - childLayout.size.width) / 2;
        childLayout.origin.y = (node.size.height - childLayout.size.height) / 2;
        node.children.push_back(childLayout);
        
        return node;
    }

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

#endif
