// =================================================================================
// Filename:    app/ConversationMessageSelection.cpp
// Author:      Ebdsaleh
// Description: Implements message-level selection helpers used by ConversationView.
// =================================================================================

#include <string.h>

#include "ConversationMessageView.h"
#include "framework/UIEvent.h"

bool ConversationMessageView::resolve_presentation_position(
    const UIEvent& event,
    int& label_index,
    int& character_index
) {
    std::vector<Label*> labels;
    collect_selectable_labels(labels);

    return resolve_selection_endpoint(
        labels,
        event,
        label_index,
        character_index
    );
}

int ConversationMessageView::get_presentation_label_count() const {
    std::vector<const Label*> labels;
    collect_selectable_labels(labels);
    return (int)labels.size();
}

int ConversationMessageView::get_presentation_label_length(
    int label_index
) const {
    std::vector<const Label*> labels;
    collect_selectable_labels(labels);

    if (label_index < 0 || label_index >= (int)labels.size()) {
        return 0;
    }

    const char* text = labels[label_index]->get_text();
    return text == 0 ? 0 : (int)strlen(text);
}

void ConversationMessageView::focus_presentation_position(
    int label_index,
    int character_index
) {
    std::vector<Label*> labels;
    collect_selectable_labels(labels);

    if (label_index < 0 || label_index >= (int)labels.size()) {
        return;
    }

    for (int index = 0; index < (int)labels.size(); ++index) {
        labels[index]->set_focused(index == label_index);
        labels[index]->clear_selection();
    }

    labels[label_index]->set_selection_range(
        character_index,
        character_index
    );
}

void ConversationMessageView::set_presentation_selection(
    int start_label_index,
    int start_character_index,
    int end_label_index,
    int end_character_index
) {
    std::vector<Label*> labels;
    collect_selectable_labels(labels);

    if (
        start_label_index < 0 ||
        start_label_index >= (int)labels.size() ||
        end_label_index < 0 ||
        end_label_index >= (int)labels.size()
    ) {
        return;
    }

    int old_anchor_label_index = selection_anchor_label_index;
    int old_anchor_character_index = selection_anchor_character_index;
    bool old_drag_selecting = presentation_drag_selecting;

    selection_anchor_label_index = start_label_index;
    selection_anchor_character_index = start_character_index;
    apply_presentation_selection(
        labels,
        end_label_index,
        end_character_index
    );

    selection_anchor_label_index = old_anchor_label_index;
    selection_anchor_character_index = old_anchor_character_index;
    presentation_drag_selecting = old_drag_selecting;
}

void ConversationMessageView::clear_presentation_selection() {
    std::vector<Label*> labels;
    collect_selectable_labels(labels);
    clear_presentation_selection(labels);
    presentation_drag_selecting = false;
    selection_anchor_label_index = -1;
    selection_anchor_character_index = 0;
}

bool ConversationMessageView::get_presentation_selection_text(
    std::string& output
) const {
    output.clear();

    std::vector<const Label*> labels;
    collect_selectable_labels(labels);

    for (int label_index = 0; label_index < (int)labels.size(); ++label_index) {
        const Label* label = labels[label_index];
        if (label == 0 || !label->has_selection()) {
            continue;
        }

        const char* label_text = label->get_text();
        if (label_text == 0) {
            continue;
        }

        int text_length = (int)strlen(label_text);
        std::string selected_text;

        for (
            int range_index = 0;
            range_index < label->get_selection_range_count();
            ++range_index
        ) {
            int start = 0;
            int end = 0;

            if (!label->get_selection_range(range_index, start, end)) {
                continue;
            }

            if (start < 0) {
                start = 0;
            }
            if (end > text_length) {
                end = text_length;
            }
            if (end <= start) {
                continue;
            }

            if (!selected_text.empty()) {
                selected_text += ' ';
            }

            selected_text.append(
                label_text + start,
                (std::string::size_type)(end - start)
            );
        }

        if (selected_text.empty()) {
            continue;
        }

        if (!output.empty()) {
            output += "\r\n";
        }

        output += selected_text;
    }

    return !output.empty();
}
