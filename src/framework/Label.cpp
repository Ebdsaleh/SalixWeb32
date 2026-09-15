// =================================================================================
// Filename:    framework/Label.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral text label component.
// =================================================================================

#include "Label.h"
#include "rendering/ComponentRenderer.h"

Label::Label()
    : horizontal_alignment(align_left) {
}

void Label::set_text(const char* new_text) {
    if (new_text == 0) {
        text.clear();
        return;
    }

    text = new_text;
}

const char* Label::get_text() const {
    return text.c_str();
}

void Label::set_horizontal_alignment(HorizontalAlignment new_alignment) {
    horizontal_alignment = new_alignment;
}

Label::HorizontalAlignment Label::get_horizontal_alignment() const {
    return horizontal_alignment;
}

void Label::render(ComponentRenderer& renderer) const {
    if (!get_is_visible()) {
        return;
    }

    renderer.render_label(*this);
}
