// =================================================================================
// Filename:    framework/rendering/ComponentRenderer.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral rendering contract for UI components.
// =================================================================================
#pragma once

class Label;

class ComponentRenderer {
    public:
        virtual ~ComponentRenderer() {}

        virtual void render_label(const Label& label) = 0;
};
