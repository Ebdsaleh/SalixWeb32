// =================================================================================
// Filename:    framework/Component.h
// Author:      Ebdsaleh
// Description: Declares the semantic base class for framework UI components.
// =================================================================================
#pragma once

class ComponentRenderer;

class Component {
    public:
        Component();
        virtual ~Component();

        void set_bounds(int x, int y, int width, int height);

        int get_x() const;
        int get_y() const;
        int get_width() const;
        int get_height() const;

        void set_visible(bool new_is_visible);
        bool get_is_visible() const;

        virtual void render(ComponentRenderer& renderer) const = 0;

    private:
        int x;
        int y;
        int width;
        int height;
        bool is_visible;
};
