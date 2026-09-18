// =================================================================================
// Filename:    conversation/backends/PlaceholderConversationBackend.h
// Author:      Ebdsaleh
// Description: Declares a local backend used to validate semantic chat streaming.
// =================================================================================
#pragma once

#include <vector>

#include "conversation/ConversationEvent.h"
#include "conversation/ConversationServiceBackend.h"

class PlaceholderConversationBackend : public ConversationServiceBackend {
    public:
        PlaceholderConversationBackend();

        virtual const char* get_name() const;
        virtual const char* get_status_text() const;

        virtual bool initialize();
        virtual void update();
        virtual void shutdown();
        virtual bool get_is_initialized() const;

        virtual bool submit_request(
            const ConversationRequest& request,
            unsigned long request_id
        );

        virtual bool take_event(ConversationEvent& event);

    private:
        enum PendingStage {
            stage_idle = 0,
            stage_request_started,
            stage_message_started,
            stage_first_delta,
            stage_second_delta,
            stage_completed
        };

        void queue_event(
            ConversationEvent::Type type,
            const char* text
        );

        bool is_initialized;
        unsigned long active_request_id;
        PendingStage pending_stage;
        std::vector<ConversationEvent> events;
};
