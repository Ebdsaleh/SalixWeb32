// =================================================================================
// Filename:    app/EmojiPanel.h
// Author:      Ebdsaleh
// Description: Declares the classic emoticon picker popup used by the composer.
// =================================================================================
#pragma once

#include <vector>

#include "framework/Panel.h"
#include "framework/Button.h"

class EmojiPanel : public Panel {
    public:
        typedef void (*EmoticonSelectedHandler)(
            EmojiPanel* panel,
            const char* alias,
            void* context
        );

        EmojiPanel();
        virtual ~EmojiPanel();

        void set_emoticon_selected_handler(
            EmoticonSelectedHandler new_handler,
            void* new_context
        );

        void set_open(bool new_is_open);
        bool get_is_open() const;

        void arrange(int x, int y, int width, int height);

    private:
        static void on_emoticon_clicked(Button* button, void* context);

        std::vector<Button*> emoticon_buttons;
        EmoticonSelectedHandler selected_handler;
        void* selected_context;
};
