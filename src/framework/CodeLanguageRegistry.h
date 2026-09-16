// =================================================================================
// Filename:    framework/CodeLanguageRegistry.h
// Author:      Ebdsaleh
// Description: Declares canonical code-language names used by composer/Markdown UI.
// =================================================================================
#pragma once

class CodeLanguageRegistry {
    public:
        static int get_count();
        static const char* get_token(int index);
        static const char* get_display_name(int index);

        static const char* canonicalize(const char* language);
        static const char* get_display_name_for_token(const char* language);
};
