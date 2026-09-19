// =================================================================================
// Filename:    conversation/ConversationServiceBackend.h
// Author:      Ebdsaleh
// Description: Declares the interchangeable semantic conversation backend contract.
// =================================================================================
#pragma once

class ConversationEvent;
class ConversationRequest;
class ConversationSecurityProfile;

class ConversationServiceBackend {
    public:
        virtual ~ConversationServiceBackend() {}

        virtual const char* get_name() const = 0;
        virtual const char* get_status_text() const = 0;

        // Optional backend-specific diagnostic detail. This is deliberately
        // separate from status text so stable UI status remains concise.
        virtual const char* get_diagnostic_text() const {
            return "";
        }

        virtual void get_security_profile(
            ConversationSecurityProfile& profile
        ) const = 0;

        virtual bool initialize() = 0;
        virtual void update() = 0;
        virtual void shutdown() = 0;
        virtual bool get_is_initialized() const = 0;

        virtual bool submit_probe(
            unsigned long request_id
        ) = 0;

        virtual bool submit_request(
            const ConversationRequest& request,
            unsigned long request_id
        ) = 0;

        virtual bool take_event(ConversationEvent& event) = 0;
};
