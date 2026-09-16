// =================================================================================
// Filename:    framework/UIEvent.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral UI input events.
// =================================================================================
#pragma once

class Clipboard;
class NativeControlHost;
class TextMetrics;

class UIEvent {
    public:
        enum Type {
            event_none = 0,
            event_mouse_move,
            event_mouse_down,
            event_mouse_up,
            event_mouse_wheel,
            event_context_menu,
            event_key_down,
            event_key_up,
            event_character,
            event_command
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
            key_escape,
            key_a,
            key_c,
            key_v,
            key_x,
            key_semicolon
        };

        UIEvent(Type new_type)
            : type(new_type),
              x(0),
              y(0),
              key_code(key_none),
              character_code(0),
              command_id(0),
              shift_down(false),
              control_down(false),
              alt_down(false),
              left_button_down(false),
              click_count(1),
              wheel_delta(0),
              clipboard(0),
              native_control_host(0),
              text_metrics(0) {
        }

        Type type;
        int x;
        int y;
        KeyCode key_code;
        int character_code;
        int command_id;
        bool shift_down;
        bool control_down;
        bool alt_down;
        bool left_button_down;
        int click_count;
        int wheel_delta;
        Clipboard* clipboard;
        NativeControlHost* native_control_host;
        TextMetrics* text_metrics;
};
