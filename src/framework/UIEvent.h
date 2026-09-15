// =================================================================================
// Filename:    framework/UIEvent.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral UI input events.
// =================================================================================
#pragma once

class UIEvent {
    public:
        enum Type {
            event_none = 0,
            event_mouse_move,
            event_mouse_down,
            event_mouse_up,
            event_key_down,
            event_key_up,
            event_character
        };

        enum KeyCode {
            key_none = 0,
            key_left,
            key_right,
            key_up,
            key_down,
            key_home,
            key_end,
            key_delete,
            key_backspace,
            key_enter,
            key_tab,
            key_escape
        };

        UIEvent(Type new_type)
            : type(new_type),
              x(0),
              y(0),
              key_code(key_none),
              character_code(0) {
        }

        Type type;
        int x;
        int y;
        KeyCode key_code;
        int character_code;
};
