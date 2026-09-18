// =================================================================================
// Filename:    conversation/ConversationServiceHost.h
// Author:      Ebdsaleh
// Description: Owns the selected semantic conversation backend lifecycle boundary.
// =================================================================================
#pragma once

class ConversationEvent;
class ConversationRequest;
class ConversationSecurityProfile;
class ConversationServiceBackend;

class ConversationServiceHost {
    public:
        ConversationServiceHost();

        bool set_backend(ConversationServiceBackend* backend);
        ConversationServiceBackend* get_backend();
        const ConversationServiceBackend* get_backend() const;

        bool initialize();
        void update();
        void shutdown();

        bool get_is_initialized() const;
        bool has_backend() const;
        const char* get_backend_name() const;
        const char* get_backend_status_text() const;
        bool get_security_profile(
            ConversationSecurityProfile& profile
        ) const;

        unsigned long submit_request(const ConversationRequest& request);
        bool take_event(ConversationEvent& event);

    private:
        ConversationServiceBackend* backend;
        bool is_initialized;
        unsigned long next_request_id;
};
