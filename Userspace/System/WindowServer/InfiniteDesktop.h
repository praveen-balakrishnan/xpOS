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

#ifndef WINDOWSERVER_INFINITEDESKTOP_H
#define WINDOWSERVER_INFINITEDESKTOP_H

#include "Window.h"
#include "Screen.h"
#include <list>

#include <API/HID.h>
#include "Geometry.h"
#include "Graphics.h"

using namespace xpOS;

class InfiniteDesktop
{
public:
    InfiniteDesktop(const RenderBuffer& buffer);

    void paint(const std::list<Rect>& dirtyRegions);
    void paint_all();

    template<class... Args>
    Window* make_window(Args&&... args) 
    {
        auto window = std::make_unique<Window>(std::forward<Args>(args)...);
        auto* raw = window.get();
        m_windows.push_back(std::move(window));
        return raw;
    }

    /**
     * Force a refresh of the window's visible area.
     */
    void invalidate_window(Window* window);
    void invalidate_windows();

    void process_mouse_event(ViewportCoord vc, int buttons);
    void process_keyboard_event(API::HID::KeyboardEvent event);

private:
    void paint_window(const Window& window);
    void paint_window(const Window& window, const ClipRects& clip);
    void fill_rect(Rect fillRect, uint32_t colour, const ClipRects& clipRects);
    void buf_rect(const Window& window, const ClipRects& clipRects);
    void draw_mouse();

    std::list<std::unique_ptr<Window>> m_windows;

    RenderBuffer m_buffer;

    Viewport m_viewport;
    RenderBuffer m_bgBuffer;

    Rect m_mouseRect;
    RenderBuffer m_mouseBuffer;

    int m_mouseButtons = 0;

    Window* m_dragWindow;
    int m_dragWindowOffsetX;
    int m_dragWindowOffsetY;

};

#endif