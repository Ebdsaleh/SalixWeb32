// =================================================================================
// Filename:    conversation/backends/RemoteConversationBackend.h
// Author:      Ebdsaleh
// Description: Declares the trusted-LAN semantic conversation probe backend.
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
        bool parse_response(
            const NetworkResponse& response,
            unsigned long expected_request_id
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
        bool request_in_flight;
        bool event_taken_this_update;
        unsigned long active_request_id;
        std::vector<ConversationEvent> events;
};
