// =================================================================================
// Filename:    app/EmoticonRegistry.cpp
// Author:      Ebdsaleh
// Description: Implements classic text-emoticon alias registration.
// =================================================================================

#include "EmoticonRegistry.h"

namespace {
    struct EmoticonDefinition {
        const char* alias;
        const char* name;
    };

    const EmoticonDefinition emoticons[] = {
        { ":)", "Smile" },
        { ":D", "Big grin" },
        { ";)", "Wink" },
        { ":P", "Tongue" },
        { ":'(", "Crying" },
        { ":O", "Surprised" },
        { "<3", "Heart" },
        { "<:", "Classic" }
    };

    const int emoticon_count = sizeof(emoticons) / sizeof(emoticons[0]);
}

int EmoticonRegistry::get_count() {
    return emoticon_count;
}

const char* EmoticonRegistry::get_alias(int index) {
    if (index < 0 || index >= emoticon_count) {
        return "";
    }

    return emoticons[index].alias;
}

const char* EmoticonRegistry::get_name(int index) {
    if (index < 0 || index >= emoticon_count) {
        return "";
    }

    return emoticons[index].name;
}
