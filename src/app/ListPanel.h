// =================================================================================
// Filename:    app/ListPanel.h
// Author:      Ebdsaleh
// Description: Declares the paragraph-list popup used by the message composer.
// =================================================================================
#pragma once

#include "framework/Panel.h"
#include "framework/Button.h"

class ListPanel : public Panel {
    public:
        enum ListStyle {
            list_clear = 0,
            list_bulleted,
            list_numbered
        };

        typedef void (*ListSelectedHandler)(
            ListPanel* panel,
            ListStyle style,
            void* context
        );

        ListPanel();

        void set_list_selected_handler(
            ListSelectedHandler new_handler,
            void* new_context
        );

        void set_open(bool new_is_open);
        bool get_is_open() const;

        void arrange(int x, int y, int width, int height);

    private:
        static void on_button_clicked(Button* button, void* context);
        void select_style(ListStyle style);

        Button bullet_button;
        Button numbered_button;
        Button clear_button;
        ListSelectedHandler selected_handler;
        void* selected_context;
};
