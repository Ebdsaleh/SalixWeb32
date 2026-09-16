// =================================================================================
// Filename:    web/platform/WebInputEvent.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral input forwarded from WebView to a backend.
// =================================================================================
#pragma once

class WebInputEvent {
    public:
        enum Type {
            input_none = 0,
            input_mouse_move,
            input_mouse_down,
            input_mouse_up,
            input_mouse_wheel,
            input_key_down,
            input_key_up,
            input_character
        };

        WebInputEvent(Type new_type = input_none)
            : type(new_type),
              x(0),
              y(0),
              key_code(0),
              character_code(0),
              wheel_delta(0),
              shift_down(false),
              control_down(false),
              alt_down(false),
              left_button_down(false) {
        }

        Type type;
        int x;
        int y;
        int key_code;
        int character_code;
        int wheel_delta;
        bool shift_down;
        bool control_down;
        bool alt_down;
        bool left_button_down;
};
