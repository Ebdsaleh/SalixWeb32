// =================================================================================
// Filename:    framework/CodeLanguageRegistry.cpp
// Author:      Ebdsaleh
// Description: Implements canonical code-language names used by composer/Markdown UI.
// =================================================================================

#include <ctype.h>
#include <string.h>
#include <string>

#include "CodeLanguageRegistry.h"

namespace {
    struct LanguageEntry {
        const char* token;
        const char* display_name;
    };

    const LanguageEntry languages[] = {
        { "", "Code" },
        { "c", "C" },
        { "cpp", "C++" },
        { "csharp", "C#" },
        { "java", "Java" },
        { "python", "Python" },
        { "javascript", "JavaScript" },
        { "typescript", "TypeScript" },
        { "json", "JSON" },
        { "bash", "Shell" },
        { "rust", "Rust" },
        { "lua", "Lua" },
        { "html", "HTML" },
        { "css", "CSS" },
        { "xml", "XML" },
        { "text", "Plain text" }
    };

    const int language_count =
        (int)(sizeof(languages) / sizeof(languages[0]));

    std::string lower_copy(const char* value) {
        std::string result = value == 0 ? "" : value;

        for (int index = 0; index < (int)result.length(); ++index) {
            result[index] = (char)tolower((unsigned char)result[index]);
        }

        return result;
    }
}

int CodeLanguageRegistry::get_count() {
    return language_count;
}

const char* CodeLanguageRegistry::get_token(int index) {
    if (index < 0 || index >= language_count) {
        return "";
    }

    return languages[index].token;
}

const char* CodeLanguageRegistry::get_display_name(int index) {
    if (index < 0 || index >= language_count) {
        return "Code";
    }

    return languages[index].display_name;
}

const char* CodeLanguageRegistry::canonicalize(const char* language) {
    std::string lower = lower_copy(language);

    if (lower.empty() || lower == "code") {
        return "";
    }

    if (lower == "c") {
        return "c";
    }

    if (lower == "cpp" || lower == "c++" || lower == "cxx") {
        return "cpp";
    }

    if (lower == "csharp" || lower == "c#" || lower == "cs") {
        return "csharp";
    }

    if (lower == "java") {
        return "java";
    }

    if (lower == "python" || lower == "py") {
        return "python";
    }

    if (lower == "javascript" || lower == "js") {
        return "javascript";
    }

    if (lower == "typescript" || lower == "ts") {
        return "typescript";
    }

    if (lower == "json") {
        return "json";
    }

    if (lower == "bash" || lower == "shell" || lower == "sh") {
        return "bash";
    }

    if (lower == "rust" || lower == "rs") {
        return "rust";
    }

    if (lower == "lua") {
        return "lua";
    }

    if (lower == "html" || lower == "htm") {
        return "html";
    }

    if (lower == "css") {
        return "css";
    }

    if (lower == "xml") {
        return "xml";
    }

    if (
        lower == "text" ||
        lower == "plaintext" ||
        lower == "plain" ||
        lower == "txt"
    ) {
        return "text";
    }

    return language == 0 ? "" : language;
}

const char* CodeLanguageRegistry::get_display_name_for_token(
    const char* language
) {
    const char* canonical = canonicalize(language);

    for (int index = 0; index < language_count; ++index) {
        if (strcmp(canonical, languages[index].token) == 0) {
            return languages[index].display_name;
        }
    }

    return language == 0 || language[0] == '\0' ? "Code" : language;
}
