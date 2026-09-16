// =================================================================================
// Filename:    framework/ScrollBar.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral scrollbar with optional native peer.
// =================================================================================
#pragma once

#include "Panel.h"
#include "Button.h"

class UIEvent;

class ScrollBar : public Panel {
    public:
        enum Orientation {
            horizontal = 0,
            vertical
        };

        typedef void (*ValueChangedHandler)(
            ScrollBar* scroll_bar,
            int value,
            void* context
        );

        ScrollBar(Orientation orientation);

        Orientation get_orientation() const;

        void set_range(int minimum, int maximum, int page_size);
        int get_minimum() const;
        int get_maximum() const;
        int get_page_size() const;

        void set_value(int new_value);
        int get_value() const;

        void set_line_step(int new_line_step);
        int get_line_step() const;

        void set_native_peer_active(bool active);
        bool get_native_peer_active() const;
        void notify_native_value_changed(int new_value);

        void set_value_changed_handler(
            ValueChangedHandler new_handler,
            void* new_context
        );

        void arrange(int x, int y, int width, int height);
        virtual bool handle_event(const UIEvent& event);

    private:
        static void on_decrement_clicked(Button* button, void* context);
        static void on_increment_clicked(Button* button, void* context);

        void change_value(int new_value, bool notify);
        void update_thumb_bounds();
        void update_fallback_visibility();
        void notify_value_changed();
        int get_axis_position(int x, int y) const;

        Orientation orientation;
        Button decrement_button;
        Button increment_button;
        Panel thumb_panel;

        int minimum;
        int maximum;
        int page_size;
        int value;
        int line_step;

        int track_start;
        int track_length;
        int thumb_start;
        int thumb_length;
        bool dragging_thumb;
        int drag_offset;
        bool native_peer_active;

        ValueChangedHandler value_changed_handler;
        void* value_changed_context;
};
