// =================================================================================
// Filename:    framework/Panel.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral styled container panel.
// =================================================================================

#include "Panel.h"
#include "rendering/ComponentRenderer.h"

Panel::Panel() {
}

Panel::~Panel() {
}

void Panel::render(ComponentRenderer& renderer) const {
    if (!get_is_visible()) {
        return;
    }

    renderer.render_panel(*this);
    Container::render(renderer);
}
