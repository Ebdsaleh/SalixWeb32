// =================================================================================
// Filename:    conversation/ConversationAttachmentPolicy.h
// Author:      Ebdsaleh
// Description: Defines bounded attachment limits shared by UI and transport layers.
// =================================================================================
#pragma once

class ConversationAttachmentPolicy {
    public:
        enum {
            maximum_attachment_count = 8
        };

        static unsigned long maximum_attachment_bytes() {
            return 2UL * 1024UL * 1024UL;
        }

        static unsigned long maximum_total_attachment_bytes() {
            return 4UL * 1024UL * 1024UL;
        }
};
