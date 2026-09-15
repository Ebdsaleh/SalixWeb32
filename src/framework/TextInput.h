// =================================================================================
// Filename:    framework/TextInput.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral single-line text input component.
// =================================================================================
#pragma once

#include <string>

#include "Component.h"

class TextInput : public Component {
    public:
        TextInput();

        void set_text(const char* new_text);
        const char* get_text() const;

        void set_max_length(int new_max_length);
        int get_max_length() const;

        void set_focused(bool new_is_focused);
        bool get_is_focused() const;

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const;

    private:
        std::string text;
        int max_length;
        bool is_focused;
};
