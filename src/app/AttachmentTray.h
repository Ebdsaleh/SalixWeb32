// =================================================================================
// Filename:    app/AttachmentTray.h
// Author:      Ebdsaleh
// Description: Declares the removable attachment queue shown in the composer.
// =================================================================================
#pragma once

#include <string>
#include <vector>

#include "framework/Panel.h"
#include "framework/Button.h"
#include "AttachmentChip.h"

class AttachmentTray : public Panel {
    public:
        typedef void (*AttachmentRemovedHandler)(
            AttachmentTray* tray,
            int index,
            void* context
        );

        AttachmentTray();
        virtual ~AttachmentTray();

        void set_paths(const std::vector<std::string>& paths);
        void clear();

        void set_attachment_removed_handler(
            AttachmentRemovedHandler new_handler,
            void* new_context
        );

        int get_attachment_count() const;
        int get_preferred_height() const;

        void arrange(int x, int y, int width, int height);

    private:
        static void on_chip_remove(
            AttachmentChip* chip,
            void* context
        );
        static void on_previous_clicked(Button* button, void* context);
        static void on_next_clicked(Button* button, void* context);

        void rebuild_chips(const std::vector<std::string>& paths);
        void layout_chips();
        void clear_chips();
        int find_chip_index(AttachmentChip* chip) const;

        std::vector<AttachmentChip*> chips;
        Button previous_button;
        Button next_button;
        AttachmentRemovedHandler attachment_removed_handler;
        void* attachment_removed_context;
        int first_visible_index;
};
