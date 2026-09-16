// =================================================================================
// Filename:    framework/rendering/ComponentRenderer.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral rendering contract for UI components.
// =================================================================================
#pragma once

class Panel;
class Label;
class Button;
class TextInput;

class ComponentRenderer {
    public:
        virtual ~ComponentRenderer() {}

        virtual void push_clip_rect(int x, int y, int width, int height) {
            (void)x;
            (void)y;
            (void)width;
            (void)height;
        }

        virtual void pop_clip_rect() {
        }

        virtual void render_panel(const Panel& panel) = 0;
        virtual void render_label(const Label& label) = 0;
        virtual void render_button(const Button& button) = 0;
        virtual void render_text_input(const TextInput& text_input) = 0;
};
