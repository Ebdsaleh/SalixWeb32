// =================================================================================
// Filename:    framework/TextViewportState.h
// Author:      Ebdsaleh
// Description: Declares shared viewport offsets for custom-rendered text inputs.
// =================================================================================
#pragma once

class TextInput;

class TextViewportState {
    public:
        static void set_scroll(
            const TextInput* text_input,
            int scroll_x,
            int scroll_y
        );

        static void get_scroll(
            const TextInput* text_input,
            int& scroll_x,
            int& scroll_y
        );

        static void clear(const TextInput* text_input);
};
