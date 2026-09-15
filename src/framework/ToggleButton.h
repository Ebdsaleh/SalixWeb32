// =================================================================================
// Filename:    framework/ToggleButton.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral toggleable button component.
// =================================================================================
#pragma once

#include "Button.h"

class ToggleButton : public Button {
    public:
        ToggleButton();

        void set_checked(bool new_is_checked);
        bool get_is_checked() const;

        void set_checked_background_color(const Color& color);
        void set_unchecked_background_color(const Color& color);

        virtual bool handle_event(const UIEvent& event);

    private:
        void update_visual_state();

        bool is_checked;
        Color checked_background_color;
        Color unchecked_background_color;
};
