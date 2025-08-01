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

#ifndef XPOS_GUILIB_STACK_H
#define XPOS_GUILIB_STACK_H

#include "View.h"

namespace xpOS::GUILib
{

class HStackView : public View
{
public:
    HStackView(std::initializer_list<std::shared_ptr<View>> views, int spacing = 0)
    : m_childViews(views)
    , m_spacing(spacing)
    {}

    LayoutNode computeLayout(const Size& suggestion) override;
    void draw(Point point, Size size, Context& context) override
    {}

private:
    std::vector<std::shared_ptr<View>> m_childViews;
    int m_spacing;
};

class VStackView : public View
{
public:
    VStackView(std::initializer_list<std::shared_ptr<View>> views, int spacing = 0)
    : m_childViews(views)
    , m_spacing(spacing)
    {}

    LayoutNode computeLayout(const Size& suggestion) override;
    void draw(Point point, Size size, Context& context) override
    {}
private:
    std::vector<std::shared_ptr<View>> m_childViews;
    int m_spacing;
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

std::shared_ptr<VStackView> VStack(std::initializer_list<std::shared_ptr<View>> views);
std::shared_ptr<HStackView> HStack(std::initializer_list<std::shared_ptr<View>> views);
std::shared_ptr<SpacerView> Spacer();

}

#endif
