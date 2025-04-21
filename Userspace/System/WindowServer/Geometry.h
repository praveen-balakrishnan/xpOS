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

#ifndef WINDOWSERVER_GEOMETRY_H
#define WINDOWSERVER_GEOMETRY_H

#include <list>

struct WorldCoord
{
    long long x = 0;
    long long y = 0;
};

struct Rect
{
    WorldCoord coord;
    int width = 0;
    int height = 0;

    long long left() const
    {
        return coord.x;
    }

    long long top() const
    {
        return coord.y;
    }

    long long right() const
    {
        return left() + width;
    }

    long long bottom() const
    {
        return top() + height;
    }

    bool contains(WorldCoord wc) const
    {
        return wc.x >= left() && wc.y >= top() && wc.x < right() && wc.y < bottom();
    }

    bool contains(const Rect& r) const
    {
        return r.left() >= left() && r.top() >= top()
                && r.right() <= right() && r.bottom() <= bottom();

    }

    bool intersects(const Rect& r) const
    {
        return (this->left() < r.right() && this->right() > r.left()
                && this->top() < r.bottom() && this->bottom() > r.top());
    }

    Rect intersect(Rect r) const
    {
        if (!this->intersects(r))
            return Rect();
        
        if (this->left() > r.left() && this->left() < r.right()) {
            r.width -= this->left() - r.left();
            r.coord.x = this->left();
        }
        
        if (this->top() > r.top() && this->top() < r.bottom()) {
            r.height -= this->top() - r.top();
            r.coord.y = this->top();
        }
        
        if (this->right() > r.left() && this->right() < r.right())
            r.width -= r.right() - this->right();

        if (this->bottom() > r.top() && this->bottom() < r.bottom())
            r.height -= r.bottom() - this->bottom();

        return r;
    }

    /**
     * Returns a list of disjoint rectangles that union together with cutting to
     * form the original rectangle.
     */
    std::list<Rect> subtract(const Rect& cutting) const
    {
        std::list<Rect> ret;
        Rect base = *this;

        if (cutting.left() > base.left() && cutting.left() < base.right()) {
            auto overlap = cutting.left() - base.left();
            Rect r = {
                .coord = base.coord,
                .width = overlap,
                .height = base.height
            };
            ret.push_back(r);
            base.coord.x += overlap;
            base.width -= overlap;
        }

        if (cutting.top() > base.top() && cutting.top() < base.bottom()) {
            auto overlap = cutting.top() - base.top();
            Rect r = {
                .coord = base.coord,
                .width = base.width,
                .height = overlap
            };
            ret.push_back(r);
            base.coord.y += overlap;
            base.height -= overlap;
        }

        if (cutting.right() > base.left() && cutting.right() < base.right()) {
            auto overlap = base.right() - cutting.right(); 
            auto coord = base.coord;
            coord.x += cutting.right() - base.left();
            Rect r = {
                .coord = coord,
                .width = overlap,
                .height = base.height
            };
            ret.push_back(r);
            base.width -= overlap;
        }

        if (cutting.bottom() > base.top() && cutting.bottom() < base.bottom()) {
            auto overlap = base.bottom() - cutting.bottom(); 
            auto coord = base.coord;
            coord.y += cutting.bottom() - base.top();
            Rect r = {
                .coord = coord,
                .width = base.width,
                .height = overlap
            };
            ret.push_back(r);
            base.height -= overlap;
        }

        return ret;
    }
};

using Viewport = Rect;

struct ViewportCoord
{
    long long x;
    long long y;

    static ViewportCoord from_world(WorldCoord wc, Viewport viewport)
    {
        return ViewportCoord {
            .x = wc.x - viewport.coord.x,
            .y = wc.y - viewport.coord.y
        };
    }

    WorldCoord world_coord(Viewport viewport)
    {
        return WorldCoord(x + viewport.coord.x, y + viewport.coord.y);
    }
};

/**
 * ClipRects are used to define a region to be operated on. The region is
 * represented by a list of disjoint rectangles, whose union is the clipping
 * region.
 */
struct ClipRects
{
    std::list<Rect> rects;

    /**
     * Remove a rectangular region from the clipping region.
     */
    void subtract(const Rect& rect)
    {
        for (auto it = rects.begin(); it != rects.end(); it++) {
            if (!it->intersects(rect))
                continue;
            
            auto split = it->subtract(rect);
            rects.erase(it);
            rects.splice(rects.end(), split);
            it = rects.begin();   
        }
    }

    /**
     * Add a rectangular region to the clipping region. The rectangle
     * will always be a seperate, disjoint rectangle in the list when added.
     */
    void add(const Rect& rect)
    {
        subtract(rect);
        rects.push_back(rect);
    }

    /**
     * Intersects the clipping region with the inputted rectangle.
     */
    void intersect(const Rect& rect)
    {
        std::list<Rect> newClips;
        for (auto& clipRect : rects) {
            if (clipRect.intersects(rect)) {
                Rect intersection = clipRect.intersect(rect);
                newClips.push_back(intersection);
            }
        }
        rects = newClips;
    }

    /**
     * Empties the clipping region.
     */
    void clear()
    {
        rects.clear();
    }

};

#endif