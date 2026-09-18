// =================================================================================
// Filename:    conversation/backends/PlaceholderConversationBackend.cpp
// Author:      Ebdsaleh
// Description: Implements a local semantic-stream contract validation backend.
// =================================================================================

#include "PlaceholderConversationBackend.h"
#include "conversation/ConversationRequest.h"
#include "conversation/ConversationSecurityProfile.h"

PlaceholderConversationBackend::PlaceholderConversationBackend()
    : is_initialized(false),
      active_request_id(0),
      pending_stage(stage_idle) {
}

const char* PlaceholderConversationBackend::get_name() const {
    return "Placeholder Conversation Backend";
}

const char* PlaceholderConversationBackend::get_status_text() const {
    return is_initialized
        ? "semantic contract ready"
        : "stopped";
}

void PlaceholderConversationBackend::get_security_profile(
    ConversationSecurityProfile& profile
) const {
    profile = ConversationSecurityProfile();
    profile.dispatch_mode = conversation_dispatch_content;
    profile.transport_security =
        conversation_transport_local_process;
    profile.text = true;
    profile.attachments = true;
}

bool PlaceholderConversationBackend::initialize() {
    if (is_initialized) {
        return true;
    }

    is_initialized = true;
    active_request_id = 0;
    pending_stage = stage_idle;
    events.clear();
    return true;
}

void PlaceholderConversationBackend::update() {
    if (!is_initialized || pending_stage == stage_idle) {
        return;
    }

    switch (pending_stage) {
        case stage_request_started:
            queue_event(
                ConversationEvent::event_request_started,
                ""
            );
            pending_stage = stage_message_started;
            break;

        case stage_message_started:
            queue_event(
                ConversationEvent::event_message_started,
                ""
            );
            pending_stage = stage_first_delta;
            break;

        case stage_first_delta:
            queue_event(
                ConversationEvent::event_text_delta,
                "**Conversation service contract online.**\n\n"
            );
            pending_stage = stage_second_delta;
            break;

        case stage_second_delta:
            queue_event(
                ConversationEvent::event_text_delta,
                "This local placeholder response arrived through semantic "
                "text-delta events. No external conversation service was contacted."
            );
            pending_stage = stage_completed;
            break;

        case stage_completed:
            queue_event(
                ConversationEvent::event_message_completed,
                ""
            );
            pending_stage = stage_idle;
            active_request_id = 0;
            break;

        case stage_idle:
        default:
            break;
    }
}

void PlaceholderConversationBackend::shutdown() {
    is_initialized = false;
    active_request_id = 0;
    pending_stage = stage_idle;
    events.clear();
}

bool PlaceholderConversationBackend::get_is_initialized() const {
    return is_initialized;
}

bool PlaceholderConversationBackend::submit_probe(
    unsigned long request_id
) {
    (void)request_id;
    return false;
}

bool PlaceholderConversationBackend::submit_request(
    const ConversationRequest& request,
    unsigned long request_id
) {
    if (
        !is_initialized ||
        request.empty() ||
        request_id == 0 ||
        pending_stage != stage_idle
    ) {
        return false;
    }

    active_request_id = request_id;
    pending_stage = stage_request_started;
    return true;
}

bool PlaceholderConversationBackend::take_event(
    ConversationEvent& event
) {
    event.clear();

    if (events.empty()) {
        return false;
    }

    event = events[0];
    events.erase(events.begin());
    return true;
}

void PlaceholderConversationBackend::queue_event(
    ConversationEvent::Type type,
    const char* text
) {
    ConversationEvent event;
    event.set_type(type);
    event.set_request_id(active_request_id);
    event.set_text(text);
    events.push_back(event);
}
