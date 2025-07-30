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

#include "Context.h"
#include "View.h"

namespace xpOS::GUILib
{

void draw(LayoutNode& tree, Context& context, Point origin)
{
    origin.x += tree.origin.x;
    origin.y += tree.origin.y;
    tree.view->draw(origin, tree.size, context);
    for (auto& child : tree.children)
        draw(child, context, origin);
}

void Context::run_view(std::shared_ptr<View> view)
{
    auto layoutTree = view->computeLayout({size.width, size.height});
    draw(layoutTree, *this, {0, 0});
}

}