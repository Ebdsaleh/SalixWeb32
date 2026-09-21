// =================================================================================
// Filename:    conversation/ConversationAttachmentPolicy.h
// Author:      Ebdsaleh
// Description: Defines bounded attachment limits shared by UI and transport layers.
// =================================================================================
#pragma once

class ConversationAttachmentPolicy {
    public:
        enum {
            maximum_attachment_count = 8,
            maximum_attachment_megabytes = 2,
            maximum_total_attachment_megabytes = 4
        };

        static unsigned long maximum_attachment_bytes() {
            return
                (unsigned long)maximum_attachment_megabytes *
                1024UL *
                1024UL;
        }

        static unsigned long maximum_total_attachment_bytes() {
            return
                (unsigned long)maximum_total_attachment_megabytes *
                1024UL *
                1024UL;
        }
};
