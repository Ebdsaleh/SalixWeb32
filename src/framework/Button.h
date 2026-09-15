// =================================================================================
// Filename:    framework/Button.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral clickable button component.
// =================================================================================
#pragma once

#include <string>

#include "Component.h"

class Button : public Component {
    public:
        typedef void (*ClickHandler)(Button* button, void* context);

        Button();

        void set_text(const char* new_text);
        const char* get_text() const;

        void set_click_handler(ClickHandler new_click_handler, void* new_context);

        void set_enabled(bool new_is_enabled);
        bool get_is_enabled() const;

        bool get_is_pressed() const;
        bool get_is_hovered() const;

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const;

    private:
        std::string text;
        bool is_enabled;
        bool is_pressed;
        bool is_hovered;
        ClickHandler click_handler;
        void* click_context;
};
