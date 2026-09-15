// =================================================================================
// Filename:    framework/TextMetrics.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral text measurement used by interactive controls.
// =================================================================================
#pragma once

class TextMetrics {
    public:
        virtual ~TextMetrics() {}

        virtual int get_character_index_at_x(
            const char* text,
            int text_length,
            int pixel_x
        ) = 0;
};
