// =================================================================================
// Filename:    framework/EmoticonRegistry.cpp
// Author:      Ebdsaleh
// Description: Implements canonical classic emoticons and alias matching helpers.
// =================================================================================

#include <string.h>

#include "EmoticonRegistry.h"

namespace {
    struct CanonicalEmoticonDefinition {
        EmoticonRegistry::EmoticonId id;
        const char* alias;
        const char* name;
        const char* icon_key;
    };

    struct EmoticonAliasDefinition {
        const char* alias;
        EmoticonRegistry::EmoticonId id;
    };

    const CanonicalEmoticonDefinition canonical_emoticons[] = {
        { EmoticonRegistry::emoticon_smile, ":)", "Smile", "smile" },
        { EmoticonRegistry::emoticon_big_grin, ":D", "Big grin", "big_grin" },
        { EmoticonRegistry::emoticon_wink, ";)", "Wink", "wink" },
        { EmoticonRegistry::emoticon_tongue, ":P", "Tongue", "tongue" },
        { EmoticonRegistry::emoticon_crying, ":'(", "Crying", "crying" },
        { EmoticonRegistry::emoticon_surprised, ":O", "Surprised", "surprised" },
        { EmoticonRegistry::emoticon_heart, "<3", "Heart", "heart" },
        { EmoticonRegistry::emoticon_classic, "<:", "Classic", "classic" }
    };

    // Long aliases come first so a visual match never consumes a shorter prefix.
    const EmoticonAliasDefinition emoticon_aliases[] = {
        { ":'-(", EmoticonRegistry::emoticon_crying },
        { ":-)", EmoticonRegistry::emoticon_smile },
        { ":-D", EmoticonRegistry::emoticon_big_grin },
        { ";-)", EmoticonRegistry::emoticon_wink },
        { ":-P", EmoticonRegistry::emoticon_tongue },
        { ":-p", EmoticonRegistry::emoticon_tongue },
        { ":-O", EmoticonRegistry::emoticon_surprised },
        { ":-o", EmoticonRegistry::emoticon_surprised },
        { ":'(", EmoticonRegistry::emoticon_crying },
        { ":)", EmoticonRegistry::emoticon_smile },
        { ":D", EmoticonRegistry::emoticon_big_grin },
        { ";)", EmoticonRegistry::emoticon_wink },
        { ":P", EmoticonRegistry::emoticon_tongue },
        { ":p", EmoticonRegistry::emoticon_tongue },
        { ":O", EmoticonRegistry::emoticon_surprised },
        { ":o", EmoticonRegistry::emoticon_surprised },
        { "<3", EmoticonRegistry::emoticon_heart },
        { "<:", EmoticonRegistry::emoticon_classic }
    };

    const int canonical_emoticon_count =
        sizeof(canonical_emoticons) / sizeof(canonical_emoticons[0]);

    const int emoticon_alias_count =
        sizeof(emoticon_aliases) / sizeof(emoticon_aliases[0]);
}

int EmoticonRegistry::get_count() {
    return canonical_emoticon_count;
}

EmoticonRegistry::EmoticonId EmoticonRegistry::get_id(int index) {
    if (index < 0 || index >= canonical_emoticon_count) {
        return emoticon_none;
    }

    return canonical_emoticons[index].id;
}

const char* EmoticonRegistry::get_alias(int index) {
    if (index < 0 || index >= canonical_emoticon_count) {
        return "";
    }

    return canonical_emoticons[index].alias;
}

const char* EmoticonRegistry::get_name(int index) {
    if (index < 0 || index >= canonical_emoticon_count) {
        return "";
    }

    return canonical_emoticons[index].name;
}

const char* EmoticonRegistry::get_icon_key(int index) {
    if (index < 0 || index >= canonical_emoticon_count) {
        return "";
    }

    return canonical_emoticons[index].icon_key;
}

bool EmoticonRegistry::find_exact_alias(
    const char* alias,
    EmoticonId& emoticon_id
) {
    emoticon_id = emoticon_none;

    if (alias == 0 || alias[0] == '\0') {
        return false;
    }

    for (int index = 0; index < emoticon_alias_count; ++index) {
        if (strcmp(alias, emoticon_aliases[index].alias) == 0) {
            emoticon_id = emoticon_aliases[index].id;
            return true;
        }
    }

    return false;
}

bool EmoticonRegistry::match_at(
    const char* text,
    int text_length,
    int position,
    EmoticonId& emoticon_id,
    int& match_length
) {
    emoticon_id = emoticon_none;
    match_length = 0;

    if (
        text == 0 ||
        text_length <= 0 ||
        position < 0 ||
        position >= text_length
    ) {
        return false;
    }

    for (int index = 0; index < emoticon_alias_count; ++index) {
        const char* alias = emoticon_aliases[index].alias;
        int alias_length = (int)strlen(alias);

        if (alias_length <= 0 || position + alias_length > text_length) {
            continue;
        }

        if (strncmp(text + position, alias, alias_length) == 0) {
            emoticon_id = emoticon_aliases[index].id;
            match_length = alias_length;
            return true;
        }
    }

    return false;
}

int EmoticonRegistry::get_visual_size(int font_size) {
    if (font_size < 1) {
        font_size = 1;
    }

    int visual_size = font_size + 8;

    if (visual_size < 18) {
        visual_size = 18;
    }

    if (visual_size > 34) {
        visual_size = 34;
    }

    return visual_size;
}
