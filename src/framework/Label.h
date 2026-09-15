// =================================================================================
// Filename:    framework/Label.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral text label component.
// =================================================================================
#pragma once

#include <string>

#include "Component.h"

class Label : public Component {
    public:
        enum HorizontalAlignment {
            align_left = 0,
            align_center,
            align_right
        };

        Label();

        void set_text(const char* new_text);
        const char* get_text() const;

        void set_horizontal_alignment(HorizontalAlignment new_alignment);
        HorizontalAlignment get_horizontal_alignment() const;

        virtual void render(ComponentRenderer& renderer) const;

    private:
        std::string text;
        HorizontalAlignment horizontal_alignment;
};
