// =================================================================================
// Filename:    framework/View.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral application view contract.
// =================================================================================
#pragma once

class ComponentRenderer;
class UIEvent;

class View {
    public:
        virtual ~View() {}

        virtual void layout(int width, int height) = 0;
        virtual bool handle_event(const UIEvent& event) = 0;
        virtual void render(ComponentRenderer& renderer) = 0;
};
