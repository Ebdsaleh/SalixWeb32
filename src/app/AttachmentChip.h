// =================================================================================
// Filename:    app/AttachmentChip.h
// Author:      Ebdsaleh
// Description: Declares one removable composer attachment chip.
// =================================================================================
#pragma once

#include <string>

#include "framework/Panel.h"
#include "framework/Label.h"
#include "framework/Button.h"

class AttachmentChip : public Panel {
    public:
        typedef void (*RemoveHandler)(
            AttachmentChip* chip,
            void* context
        );

        AttachmentChip();

        void set_path(const char* new_path);
        const char* get_path() const;
        const char* get_file_name() const;

        void set_remove_handler(
            RemoveHandler new_handler,
            void* new_context
        );

        int get_preferred_width() const;
        void arrange(int x, int y, int width, int height);

    private:
        static void on_remove_clicked(Button* button, void* context);

        std::string path;
        std::string file_name;
        Label file_name_label;
        Button remove_button;
        RemoveHandler remove_handler;
        void* remove_context;
};
