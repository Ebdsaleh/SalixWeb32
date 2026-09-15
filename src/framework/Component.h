// =================================================================================
// Filename:    framework/Component.h
// Author:      Ebdsaleh
// Description: Declares the semantic base class for framework UI components.
// =================================================================================
#pragma once

#include "Style.h"

class ComponentRenderer;
class UIEvent;

class Component {
    public:
        Component();
        virtual ~Component();

        void set_bounds(int x, int y, int width, int height);

        int get_x() const;
        int get_y() const;
        int get_width() const;
        int get_height() const;

        bool contains_point(int point_x, int point_y) const;

        void set_visible(bool new_is_visible);
        bool get_is_visible() const;

        ComponentStyle& get_style();
        const ComponentStyle& get_style() const;

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const = 0;

    private:
        int x;
        int y;
        int width;
        int height;
        bool is_visible;
        ComponentStyle style;
};
