// =================================================================================
// Filename:    framework/View.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral application view contract.
// =================================================================================
#pragma once

class ComponentRenderer;

class View {
    public:
        virtual ~View() {}

        virtual void layout(int width, int height) = 0;
        virtual void render(ComponentRenderer& renderer) = 0;
};
