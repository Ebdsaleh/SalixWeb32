// =================================================================================
// Filename:    app/MessageToolbar.h
// Author:      Ebdsaleh
// Description: Declares the formatting/attachment toolbar above the message input.
// =================================================================================
#pragma once

#include <string>
#include <vector>

#include "framework/Panel.h"
#include "framework/Button.h"
#include "framework/ToggleButton.h"
#include "framework/ComboBox.h"
#include "framework/Label.h"
#include "EmojiPanel.h"

class FileDialog;
class NativeControlHost;

class MessageToolbar : public Panel {
    public:
        typedef void (*InsertTextHandler)(
            MessageToolbar* toolbar,
            const char* text,
            void* context
        );

        typedef void (*AttachmentsAddedHandler)(
            MessageToolbar* toolbar,
            const std::vector<std::string>& paths,
            void* context
        );

        typedef void (*FormatChangedHandler)(
            MessageToolbar* toolbar,
            bool bold,
            bool italic,
            bool underline,
            int font_size,
            void* context
        );

        MessageToolbar(FileDialog* file_dialog);
        virtual ~MessageToolbar();

        void set_file_dialog(FileDialog* new_file_dialog);

        void attach_native_controls(NativeControlHost* control_host);
        void detach_native_controls();

        void set_insert_text_handler(
            InsertTextHandler new_handler,
            void* new_context
        );

        void set_attachments_added_handler(
            AttachmentsAddedHandler new_handler,
            void* new_context
        );

        void set_format_changed_handler(
            FormatChangedHandler new_handler,
            void* new_context
        );

        void set_attachment_count(int attachment_count);

        bool get_bold() const;
        bool get_italic() const;
        bool get_underline() const;
        int get_font_size() const;

        bool contains_popup_point(int x, int y) const;

        void arrange(int x, int y, int width, int height);

        virtual bool handle_event(const UIEvent& event);

    private:
        static void on_font_size_changed(
            ComboBox* combo_box,
            int selected_value,
            const char* selected_text,
            void* context
        );

        static void on_emoticon_selected(
            EmojiPanel* panel,
            const char* alias,
            void* context
        );

        void toggle_bold();
        void toggle_italic();
        void toggle_underline();
        void notify_format_changed();
        bool open_attachment_dialog();

        FileDialog* file_dialog;
        NativeControlHost* native_control_host;
        InsertTextHandler insert_text_handler;
        void* insert_text_context;
        AttachmentsAddedHandler attachments_added_handler;
        void* attachments_added_context;
        FormatChangedHandler format_changed_handler;
        void* format_changed_context;

        Button attach_button;
        ToggleButton bold_button;
        ToggleButton italic_button;
        ToggleButton underline_button;
        ComboBox font_size_combo;
        Button list_button;
        Button emoji_button;
        Label attachment_status_label;
        EmojiPanel emoji_panel;

        int font_size;
};
