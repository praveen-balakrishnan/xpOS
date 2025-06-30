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

#include "InfiniteDesktop.h"
#include "Graphics.h"
#include <ranges>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

InfiniteDesktop::InfiniteDesktop(const RenderBuffer& buffer)
    : m_buffer(buffer)
{
    m_viewport = Viewport {
        .coord = WorldCoord {.x = 0, .y = 0},
        .width = m_buffer.width,
        .height = m_buffer.height,
    };
    unsigned char *data = stbi_load(
        "xpinitrd/System/Resources/SmallCursor.png",
        &m_mouseRect.width,
        &m_mouseRect.height,
        nullptr,
        4
    );

    m_mouseBuffer.width = m_mouseRect.width;
    m_mouseBuffer.height = m_mouseRect.height;
    m_mouseBuffer.buffer = reinterpret_cast<uint32_t*>(data);

    data = stbi_load(
        "xpinitrd/System/Resources/Wallpaper.png",
        &m_bgBuffer.width,
        &m_bgBuffer.height,
        nullptr,
        3
    );

    auto* pixelBuf = new uint32_t[m_bgBuffer.width * m_bgBuffer.height];
    // We need to switch from RGBA to ARGB.
    for (int i = 0; i < m_bgBuffer.width * m_bgBuffer.height; i++) {
        pixelBuf[i] = (0xFF << 24) | (data[i*3] << 16) | (data[i*3 + 1] << 8) | data[i*3+2];
    }

    m_bgBuffer.buffer = pixelBuf;
}

void InfiniteDesktop::paint_all()
{
    std::list<Rect> dirty;
    dirty.push_back(m_viewport);
    paint(dirty);
}

void InfiniteDesktop::invalidate_windows()
{
    for (auto& window : m_windows)
        invalidate_window(window.get());
}

void InfiniteDesktop::paint(const std::list<Rect>& dirtyRegions)
{
    ClipRects desktopClip;
    // We first draw the parts of the desktop that are dirty and not occluded.
    for (auto& dirtyRegion : dirtyRegions)
        desktopClip.add(dirtyRegion);
        
    desktopClip.intersect(m_viewport);

    for (auto& window : m_windows) 
        desktopClip.subtract(window->get_geometry());

    for (auto& clipRect : desktopClip.rects) {
        BitmapGraphics::buf_rect(
            m_buffer,
            m_bgBuffer.buffer,
            m_viewport,
            clipRect,
            m_viewport
        );
    }

    ClipRects windowClip;

    for (auto it = m_windows.begin(); it != m_windows.end(); it++) {
        auto windowGeometry = (*it)->get_geometry();

        for (auto& dirtyRegion : dirtyRegions)
            windowClip.add(dirtyRegion);
        
        windowClip.intersect(windowGeometry);
        windowClip.intersect(m_viewport);

        // For each window clip, we subtract the region of windows above it
        // that occlude it.

        auto windowsAboveIt = it;
        windowsAboveIt++;
        for (; windowsAboveIt != m_windows.end(); windowsAboveIt++) {
            if ((*windowsAboveIt)->get_geometry().intersects(windowGeometry))
                windowClip.subtract((*windowsAboveIt)->get_geometry());
        }
        paint_window(*it->get(), windowClip);
        windowClip.clear();
    }
}

void InfiniteDesktop::paint_window(const Window& window, const ClipRects& clip)
{
    buf_rect(window, clip);
}

void InfiniteDesktop::paint_window(const Window& window)
{
    ClipRects clip;
    clip.add(window.get_geometry());
    clip.intersect(m_viewport);
    paint_window(window, clip);
}

void InfiniteDesktop::invalidate_window(Window* window)
{
    ClipRects windowClip;
   
    auto windowGeometry = window->get_geometry();
    
    windowClip.add(windowGeometry);
    windowClip.intersect(m_viewport);

    // For each window clip, we subtract the region of windows above it
    // that occlude it.

    auto windowsAboveIt = m_windows.begin();
    while (windowsAboveIt->get() != window)
        windowsAboveIt++;
    
    windowsAboveIt++;

    for (; windowsAboveIt != m_windows.end(); windowsAboveIt++) {
        auto windowAboveGeo = (*windowsAboveIt)->get_geometry();
        if (windowAboveGeo.intersects(windowGeometry))
                windowClip.subtract(windowAboveGeo);
    }
    paint_window(*window, windowClip);
    draw_mouse();
    Screen::main().flush();
}

void InfiniteDesktop::process_mouse_event(ViewportCoord vc, int buttons)
{
    WorldCoord cursorCoord = vc.world_coord(m_viewport);
    if (buttons & API::HID::MouseButtons::LEFTMB && m_windows.size() > 0) {
        if (!(m_mouseButtons & API::HID::MouseButtons::LEFTMB)) {
            if (!m_windows.back()->get_geometry().contains(cursorCoord)) {
                for (auto rit = m_windows.rbegin(); rit != m_windows.rend(); rit++) {
                    if (rit->get()->get_geometry().contains(cursorCoord)) {
                        // The window list is organised bottom to top in z-position.
                        // We therefore move the clicked window to the end to raise it.
                        std::advance(rit, 1);
                        m_windows.splice(m_windows.end(), m_windows, rit.base());
                        break;
                    }
                }
            }
            auto winGeo = m_windows.back()->get_geometry();
            if (winGeo.contains(cursorCoord)) {
                m_dragWindow = m_windows.back().get();
                m_dragWindowOffsetX = cursorCoord.x - winGeo.coord.x;
                m_dragWindowOffsetY = cursorCoord.y - winGeo.coord.y;
            }
        }
    } else {
        m_dragWindow = nullptr;
    }

    ClipRects dirty;
    if (m_dragWindow) {
        // Subtract the old window position, move it and then paint it.
        Rect oldPosition = m_dragWindow->get_geometry();
        dirty.add(oldPosition);
        WorldCoord newPos {
            .x = cursorCoord.x - m_dragWindowOffsetX,
            .y = cursorCoord.y - m_dragWindowOffsetY
        };
        m_dragWindow->move_window_to(newPos);
        dirty.subtract(m_dragWindow->get_geometry());
        dirty.intersect(m_viewport);
        paint_window(*m_dragWindow);
        paint(dirty.rects);
        dirty.clear();
    }

    dirty.add(m_mouseRect);
    dirty.intersect(m_viewport);
    paint(dirty.rects);

    m_mouseRect.coord = cursorCoord;
    m_mouseButtons = buttons;
    draw_mouse();
}

void InfiniteDesktop::process_keyboard_event(API::HID::KeyboardEvent event)
{
    m_windows.back()->get_context()->handle_keyboard_event(event);
}

void InfiniteDesktop::draw_mouse()
{
    BitmapGraphics::buf_rect<true>(
        m_buffer,
        m_mouseBuffer.buffer,
        m_mouseRect,
        m_viewport,
        m_viewport
    );
}

void InfiniteDesktop::fill_rect(Rect fillRect, uint32_t colour, const ClipRects& clipRects)
{
    for (auto& clipRect : clipRects.rects)
        BitmapGraphics::fill_rect(m_buffer, clipRect.intersect(fillRect), m_viewport, colour);
}

void InfiniteDesktop::buf_rect(const Window& window, const ClipRects& clipRects)
{
    auto* src = window.get_context()->get_buffer().buffer;
    auto windowRect = window.get_geometry();
    for (auto& clipRect : clipRects.rects)
        BitmapGraphics::buf_rect(m_buffer, src, windowRect, clipRect, m_viewport);
}
