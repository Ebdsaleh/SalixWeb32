// =================================================================================
// Filename:    framework/MimeTypes.h
// Author:      Ebdsaleh
// Description: Declares MIME identifiers shared by framework data-transfer paths.
// =================================================================================
#pragma once

namespace MimeTypes {
    inline const char* text_plain() {
        return "text/plain";
    }

    inline const char* salix_selection_preserved() {
        return "application/x-salixweb32-selection-preserved";
    }
}
