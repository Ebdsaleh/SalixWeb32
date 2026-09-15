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

        UIEvent(Type new_type)
            : type(new_type),
              x(0),
              y(0),
              key_code(0),
              character_code(0) {
        }

        Type type;
        int x;
        int y;
        int key_code;
        int character_code;
};
