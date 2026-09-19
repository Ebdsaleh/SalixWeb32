// =================================================================================
// Filename:    conversation/backends/RemoteConversationBackend.h
// Author:      Ebdsaleh
// Description: Declares the trusted-LAN browser conversation relay backend.
// =================================================================================
#pragma once

#include <string>
#include <vector>

#include "conversation/ConversationEvent.h"
#include "conversation/ConversationServiceBackend.h"

class NetworkRequestExecutor;
class NetworkResponse;

class RemoteConversationBackend : public ConversationServiceBackend {
    public:
        RemoteConversationBackend(
            NetworkRequestExecutor* request_executor,
            const char* host,
            unsigned short port
        );

        virtual const char* get_name() const;
        virtual const char* get_status_text() const;
        virtual const char* get_diagnostic_text() const;
        virtual void get_security_profile(
            ConversationSecurityProfile& profile
        ) const;

        virtual bool initialize();
        virtual void update();
        virtual void shutdown();
        virtual bool get_is_initialized() const;

        virtual bool submit_probe(
            unsigned long request_id
        );

        virtual bool submit_request(
            const ConversationRequest& request,
            unsigned long request_id
        );

        virtual bool take_event(ConversationEvent& event);

    private:
        enum PendingOperation {
            operation_none = 0,
            operation_health,
            operation_conversation
        };

        enum CapabilityState {
            capability_unknown = 0,
            capability_checking,
            capability_ready,
            capability_incompatible,
            capability_unreachable
        };

        bool begin_health_check();
        void apply_health_response(
            const NetworkResponse& response
        );
        bool parse_response(
            const NetworkResponse& response,
            unsigned long expected_request_id
        );
        void set_capability_status(
            CapabilityState state,
            const char* text
        );
        void queue_failure(
            unsigned long request_id,
            const char* detail
        );

        NetworkRequestExecutor* request_executor;
        std::string host;
        unsigned short port;
        bool is_initialized;
        bool bridge_online;
        bool event_taken_this_update;
        PendingOperation pending_operation;
        CapabilityState capability_state;
        unsigned long active_request_id;
        std::string status_text;
        std::string diagnostic_text;
        std::vector<ConversationEvent> events;
};
