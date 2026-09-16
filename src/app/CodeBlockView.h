// =================================================================================
// Filename:    app/CodeBlockView.h
// Author:      Ebdsaleh
// Description: Declares a dedicated conversation code-block component.
// =================================================================================
#pragma once

#include <string>

#include "framework/Panel.h"
#include "framework/Label.h"
#include "framework/Button.h"
#include "framework/FormattedText.h"
#include "framework/ScrollBar.h"

class Clipboard;
class ComponentRenderer;
class NativeControlHost;
class TextMetrics;
class UIEvent;

class CodeBlockView : public Panel {
    public:
        CodeBlockView();
        virtual ~CodeBlockView();

        void set_code(
            const FormattedText& new_code,
            const char* new_language
        );

        const char* get_code() const;
        const char* get_language() const;

        Label* get_selectable_label() {
            return &code_label;
        }

        const Label* get_selectable_label() const {
            return &code_label;
        }

        bool contains_code_text_point(int x, int y) const {
            int left = body_panel.get_x() + body_padding;
            int top = body_panel.get_y() + body_padding;
            int right = body_panel.get_x() + body_panel.get_width() - body_padding;
            int bottom = top + content_height;

            return
                x >= left &&
                x < right &&
                y >= top &&
                y < bottom;
        }

        void attach_native_controls(NativeControlHost* control_host);
        void detach_native_controls();

        int measure_height(int width, TextMetrics* text_metrics);
        void arrange(
            int x,
            int y,
            int width,
            int height,
            TextMetrics* text_metrics = 0
        );

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const;

    private:
        static void on_copy_clicked(Button* button, void* context);
        static void on_scroll_changed(
            ScrollBar* scroll_bar,
            int value,
            void* context
        );

        void rebuild_formatted_code();
        void update_language_label();
        void update_content_metrics(TextMetrics* text_metrics);
        void layout_children();
        void sync_native_scrollbar();
        void copy_all_code();

        std::string source_code;
        std::string language;
        FormattedText formatted_code;

        Panel header_panel;
        Panel body_panel;
        Label language_label;
        Button copy_button;
        Label code_label;
        ScrollBar horizontal_scroll_bar;

        NativeControlHost* native_control_host;
        Clipboard* active_clipboard;

        int header_height;
        int block_padding;
        int body_padding;
        int block_spacing;
        int scroll_bar_height;

        int content_width;
        int content_height;
        int viewport_width;
        int scroll_offset_x;
        bool layout_dirty;
};
