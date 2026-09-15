// =================================================================================
// Filename:    app/EmoticonRegistry.h
// Author:      Ebdsaleh
// Description: Declares classic text-emoticon aliases used by the composer.
// =================================================================================
#pragma once

class EmoticonRegistry {
    public:
        static int get_count();
        static const char* get_alias(int index);
        static const char* get_name(int index);
};
