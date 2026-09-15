// =================================================================================
// Filename:    framework/StackPanel.h
// Author:      Ebdsaleh
// Description: Declares a simple row/column stack layout container.
// =================================================================================
#pragma once

#include "Container.h"

class StackPanel : public Container {
    public:
        enum Orientation {
            orientation_vertical = 0,
            orientation_horizontal
        };

        StackPanel();

        void set_orientation(Orientation new_orientation);
        Orientation get_orientation() const;

        void set_item_extent(int new_item_extent);
        int get_item_extent() const;

        void set_spacing(int new_spacing);
        int get_spacing() const;

        void arrange(int x, int y, int width, int height);

    private:
        void arrange_vertical();
        void arrange_horizontal();

        Orientation orientation;
        int item_extent;
        int spacing;
};
