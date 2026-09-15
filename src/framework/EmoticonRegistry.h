// =================================================================================
// Filename:    framework/EmoticonRegistry.h
// Author:      Ebdsaleh
// Description: Declares canonical classic emoticons and alias matching helpers.
// =================================================================================
#pragma once

#include <string.h>

class EmoticonRegistry {
    public:
        enum EmoticonId {
            emoticon_none = 0,
            emoticon_smile,
            emoticon_big_grin,
            emoticon_wink,
            emoticon_tongue,
            emoticon_crying,
            emoticon_surprised,
            emoticon_heart,
            emoticon_classic
        };

        static int get_count();
        static EmoticonId get_id(int index);
        static const char* get_alias(int index);
        static const char* get_name(int index);
        static const char* get_icon_key(int index);

        static bool find_exact_alias(
            const char* alias,
            EmoticonId& emoticon_id
        );

        static bool match_at(
            const char* text,
            int text_length,
            int position,
            EmoticonId& emoticon_id,
            int& match_length
        );

        static int get_visual_size(int font_size);
};
