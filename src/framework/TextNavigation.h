// =================================================================================
// Filename:    framework/TextNavigation.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral text cursor navigation helpers.
// =================================================================================
#pragma once

#include <string>

namespace TextNavigation {
    int find_word_boundary_left(
        const std::string& text,
        int position
    );

    int find_word_boundary_right(
        const std::string& text,
        int position
    );
}
