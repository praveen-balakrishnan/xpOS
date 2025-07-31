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

#include "Libraries/GUILib/Window.h"
#include "Libraries/GUILib/Image.h"
#include "Libraries/GUILib/Stack.h"

int main()
{
    using namespace xpOS::GUILib;

    Window window = Window::create(500, 700).value();

    auto view =
        VStack({
            Spacer(),
            Image("xpinitrd/System/Resources/Les.png"),
            Spacer()
        });
    
    window.context().run_view(view);
    while(1);
}
