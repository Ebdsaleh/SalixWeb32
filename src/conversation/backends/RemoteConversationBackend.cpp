// =================================================================================
// Filename:    conversation/backends/RemoteConversationBackend.cpp
// Author:      Ebdsaleh
// Description: Implements the trusted-LAN semantic conversation probe backend.
// =================================================================================

#include <stdio.h>
#include <stdlib.h>

#include "RemoteConversationBackend.h"
#include "conversation/ConversationRequest.h"
#include "web/network/NetworkRequest.h"
#include "web/network/NetworkRequestExecutor.h"
#include "web/network/NetworkResponse.h"

namespace {
    const char* conversation_protocol = "SALIX-CONVERSATION/1";

    std::string get_protocol_value(
        const std::string& metadata,
        const char* key
    ) {
        if (key == 0 || key[0] == '\0') {
            return "";
        }

        std::string prefix(key);
        prefix += "=";

        std::string::size_type position = metadata.find(prefix);

        while (position != std::string::npos) {
            if (position == 0 || metadata[position - 1] == '\n') {
                std::string::size_type value_start =
                    position + prefix.size();
                std::string::size_type value_end =
                    metadata.find('\n', value_start);

                if (value_end == std::string::npos) {
                    value_end = metadata.size();
                }

                return metadata.substr(
                    value_start,
                    value_end - value_start
                );
            }

            position = metadata.find(prefix, position + 1);
        }

        return "";
    }

    unsigned long get_protocol_unsigned(
        const std::string& metadata,
        const char* key
    ) {
        std::string value = get_protocol_value(metadata, key);

        if (value.empty()) {
            return 0;
        }

        return strtoul(value.c_str(), 0, 10);
    }

    ConversationEvent::Type get_event_type(
        const std::string& name
    ) {
        if (name == "request_started") {
            return ConversationEvent::event_request_started;
        }

        if (name == "message_started") {
            return ConversationEvent::event_message_started;
        }

        if (name == "text_delta") {
            return ConversationEvent::event_text_delta;
        }

        if (name == "message_completed") {
            return ConversationEvent::event_message_completed;
        }

        if (name == "request_failed") {
            return ConversationEvent::event_request_failed;
        }

        return ConversationEvent::event_none;
    }

    bool parse_conversation_payload(
        const std::string& payload,
        unsigned long expected_request_id,
        std::vector<ConversationEvent>& output
    ) {
        output.clear();

        std::string::size_type header_end = payload.find("\n\n");
        if (header_end == std::string::npos) {
            return false;
        }

        std::string metadata = payload.substr(0, header_end);
        std::string protocol_line(conversation_protocol);
        std::string::size_type first_line_end =
            metadata.find('\n');

        if (
            first_line_end == std::string::npos ||
            metadata.substr(0, first_line_end) != protocol_line
        ) {
            return false;
        }

        if (get_protocol_value(metadata, "status") != "ok") {
            return false;
        }

        unsigned long request_id =
            get_protocol_unsigned(metadata, "request_id");

        if (
            request_id == 0 ||
            request_id != expected_request_id
        ) {
            return false;
        }

        unsigned long event_count =
            get_protocol_unsigned(metadata, "event_count");

        if (event_count == 0 || event_count > 32) {
            return false;
        }

        std::string::size_type cursor = header_end + 2;

        for (
            unsigned long index = 0;
            index < event_count;
            ++index
        ) {
            char type_key[64];
            char length_key[64];

            sprintf(
                type_key,
                "event_%lu_type",
                index
            );
            sprintf(
                length_key,
                "event_%lu_len",
                index
            );

            std::string type_name =
                get_protocol_value(metadata, type_key);
            ConversationEvent::Type type =
                get_event_type(type_name);

            if (type == ConversationEvent::event_none) {
                return false;
            }

            std::string length_value =
                get_protocol_value(metadata, length_key);

            if (length_value.empty()) {
                return false;
            }

            unsigned long text_length =
                strtoul(length_value.c_str(), 0, 10);

            if (
                cursor > payload.size() ||
                text_length >
                (unsigned long)(payload.size() - cursor)
            ) {
                return false;
            }

            std::string event_text(
                payload,
                cursor,
                (std::string::size_type)text_length
            );
            cursor += (std::string::size_type)text_length;

            ConversationEvent event;
            event.set_type(type);
            event.set_request_id(request_id);
            event.set_text(event_text.c_str());
            output.push_back(event);
        }

        if (
            cursor != payload.size() ||
            output.empty() ||
            output[0].get_type() !=
                ConversationEvent::event_request_started ||
            output[output.size() - 1].get_type() !=
                ConversationEvent::event_message_completed
        ) {
            output.clear();
            return false;
        }

        return true;
    }
}

RemoteConversationBackend::RemoteConversationBackend(
    NetworkRequestExecutor* new_request_executor,
    const char* new_host,
    unsigned short new_port
) : request_executor(new_request_executor),
    host(new_host == 0 ? "" : new_host),
    port(new_port),
    is_initialized(false),
    bridge_online(false),
    request_in_flight(false),
    event_taken_this_update(false),
    active_request_id(0) {
}

const char* RemoteConversationBackend::get_name() const {
    return "Remote Conversation Bridge Backend";
}

bool RemoteConversationBackend::initialize() {
    if (is_initialized) {
        return true;
    }

    if (
        request_executor == 0 ||
        host.empty() ||
        port == 0
    ) {
        return false;
    }

    if (!request_executor->initialize()) {
        return false;
    }

    bridge_online = false;
    request_in_flight = false;
    event_taken_this_update = false;
    active_request_id = 0;
    events.clear();
    is_initialized = true;
    return true;
}

void RemoteConversationBackend::update() {
    event_taken_this_update = false;

    if (
        !is_initialized ||
        request_executor == 0 ||
        !request_in_flight
    ) {
        return;
    }

    NetworkResponse response;
    std::string error_text;
    bool succeeded = false;

    if (!request_executor->take_result(
            response,
            error_text,
            succeeded
        )) {
        return;
    }

    request_in_flight = false;

    if (!succeeded) {
        bridge_online = false;
        queue_failure(
            active_request_id,
            error_text.empty()
                ? "Remote conversation probe transport failed."
                : error_text.c_str()
        );
        active_request_id = 0;
        return;
    }

    bridge_online = true;

    if (!parse_response(response, active_request_id)) {
        active_request_id = 0;
        return;
    }

    active_request_id = 0;
}

void RemoteConversationBackend::shutdown() {
    if (
        request_executor != 0 &&
        request_executor->get_is_initialized()
    ) {
        request_executor->shutdown();
    }

    is_initialized = false;
    bridge_online = false;
    request_in_flight = false;
    event_taken_this_update = false;
    active_request_id = 0;
    events.clear();
}

bool RemoteConversationBackend::get_is_initialized() const {
    return is_initialized;
}

bool RemoteConversationBackend::submit_request(
    const ConversationRequest& request,
    unsigned long request_id
) {
    if (
        !is_initialized ||
        request_executor == 0 ||
        request.empty() ||
        request_id == 0 ||
        request_in_flight ||
        !events.empty() ||
        request_executor->get_is_busy()
    ) {
        return false;
    }

    // SECURITY: This first remote proof deliberately does not serialize the
    // draft text, attachment count, attachment paths, credentials, cookies,
    // or session data.  Only the Salix request ID and fixed probe flags cross
    // the current plaintext trusted-LAN transport.
    char body[256];
    sprintf(
        body,
        "%s\n"
        "mode=probe\n"
        "request_id=%lu\n"
        "text_forwarded=0\n"
        "attachments_forwarded=0\n",
        conversation_protocol,
        request_id
    );

    NetworkRequest network_request(
        "POST",
        "/v1/conversation/probe"
    );
    network_request.set_content_type(
        "text/plain; charset=utf-8"
    );
    network_request.set_body(body);

    if (!request_executor->submit(
            host.c_str(),
            port,
            network_request
        )) {
        return false;
    }

    active_request_id = request_id;
    request_in_flight = true;
    return true;
}

bool RemoteConversationBackend::take_event(
    ConversationEvent& event
) {
    event.clear();

    if (
        event_taken_this_update ||
        events.empty()
    ) {
        return false;
    }

    event = events[0];
    events.erase(events.begin());
    event_taken_this_update = true;
    return true;
}

bool RemoteConversationBackend::parse_response(
    const NetworkResponse& response,
    unsigned long expected_request_id
) {
    if (!response.get_is_success()) {
        char detail[192];
        sprintf(
            detail,
            "Remote conversation probe returned HTTP %d.",
            response.get_status_code()
        );
        queue_failure(expected_request_id, detail);
        return false;
    }

    std::vector<ConversationEvent> parsed_events;

    if (!parse_conversation_payload(
            response.get_body(),
            expected_request_id,
            parsed_events
        )) {
        queue_failure(
            expected_request_id,
            "Remote conversation probe returned an invalid semantic payload."
        );
        return false;
    }

    events.insert(
        events.end(),
        parsed_events.begin(),
        parsed_events.end()
    );
    return true;
}

void RemoteConversationBackend::queue_failure(
    unsigned long request_id,
    const char* detail
) {
    ConversationEvent event;
    event.set_type(ConversationEvent::event_request_failed);
    event.set_request_id(request_id);
    event.set_text(
        detail == 0 || detail[0] == '\0'
            ? "Remote conversation probe failed."
            : detail
    );
    events.push_back(event);
}
