// =================================================================================
// Filename:    conversation/backends/RemoteConversationBackend.cpp
// Author:      Ebdsaleh
// Description: Implements the trusted-LAN browser conversation relay backend.
// =================================================================================

#include <stdio.h>
#include <stdlib.h>

#include "RemoteConversationBackend.h"
#include "conversation/ConversationRequest.h"
#include "conversation/ConversationSecurityProfile.h"
#include "web/network/NetworkRequest.h"
#include "web/network/NetworkRequestExecutor.h"
#include "web/network/NetworkResponse.h"
#include "conversation/ConversationAttachmentPolicy.h"

namespace {
    const char* bridge_protocol = "SALIX-BRIDGE/1";
    const char* conversation_protocol = "SALIX-CONVERSATION/1";
    const char* attachment_protocol = "SALIX-ATTACHMENT/1";

    struct OutgoingAttachment {
        std::string name;
        std::string mime_type;
        std::string data;
    };

    std::string get_file_name(const char* path) {
        if (path == 0 || path[0] == '\0') {
            return "";
        }

        std::string value(path);
        std::string::size_type separator =
            value.find_last_of("\\/");

        return separator == std::string::npos
            ? value
            : value.substr(separator + 1);
    }

    std::string lower_copy(const std::string& value) {
        std::string result(value);

        for (std::string::size_type index = 0;
             index < result.size();
             ++index) {
            char character = result[index];
            if (character >= 'A' && character <= 'Z') {
                result[index] = (char)(
                    character - 'A' + 'a'
                );
            }
        }

        return result;
    }

    std::string get_attachment_mime_type(
        const std::string& file_name
    ) {
        std::string lower_name = lower_copy(file_name);
        std::string::size_type dot =
            lower_name.find_last_of('.');

        if (dot == std::string::npos) {
            return "application/octet-stream";
        }

        std::string extension = lower_name.substr(dot);

        if (extension == ".txt" || extension == ".log") {
            return "text/plain";
        }
        if (extension == ".md") {
            return "text/markdown";
        }
        if (extension == ".csv") {
            return "text/csv";
        }
        if (extension == ".json") {
            return "application/json";
        }
        if (extension == ".xml") {
            return "application/xml";
        }
        if (extension == ".html" || extension == ".htm") {
            return "text/html";
        }
        if (extension == ".css") {
            return "text/css";
        }
        if (
            extension == ".c" ||
            extension == ".cc" ||
            extension == ".cpp" ||
            extension == ".cxx" ||
            extension == ".h" ||
            extension == ".hh" ||
            extension == ".hpp" ||
            extension == ".py" ||
            extension == ".js" ||
            extension == ".lua" ||
            extension == ".rs" ||
            extension == ".ini" ||
            extension == ".cfg" ||
            extension == ".conf" ||
            extension == ".toml" ||
            extension == ".yaml" ||
            extension == ".yml"
        ) {
            return "text/plain";
        }
        if (extension == ".png") {
            return "image/png";
        }
        if (extension == ".jpg" || extension == ".jpeg") {
            return "image/jpeg";
        }
        if (extension == ".gif") {
            return "image/gif";
        }
        if (extension == ".bmp") {
            return "image/bmp";
        }
        if (extension == ".tif" || extension == ".tiff") {
            return "image/tiff";
        }
        if (extension == ".pdf") {
            return "application/pdf";
        }
        if (extension == ".zip") {
            return "application/zip";
        }

        return "application/octet-stream";
    }

    bool read_attachment_file(
        const char* path,
        std::string& data
    ) {
        data.clear();

        if (path == 0 || path[0] == '\0') {
            return false;
        }

        FILE* file = fopen(path, "rb");
        if (file == 0) {
            return false;
        }

        if (fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            return false;
        }

        long length = ftell(file);
        if (
            length < 0 ||
            (unsigned long)length > ConversationAttachmentPolicy::maximum_attachment_bytes()
        ) {
            fclose(file);
            return false;
        }

        if (fseek(file, 0, SEEK_SET) != 0) {
            fclose(file);
            return false;
        }

        if (length > 0) {
            std::vector<char> buffer(
                (std::vector<char>::size_type)length
            );

            size_t read_count = fread(
                &buffer[0],
                1,
                (size_t)length,
                file
            );

            if (read_count != (size_t)length) {
                fclose(file);
                return false;
            }

            data.assign(&buffer[0], (std::string::size_type)length);
        }

        fclose(file);
        return true;
    }

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

    bool has_exact_protocol_line(
        const std::string& text,
        const char* protocol
    ) {
        if (protocol == 0 || protocol[0] == '\0') {
            return false;
        }

        std::string::size_type first_line_end = text.find('\n');
        std::string first_line =
            first_line_end == std::string::npos
                ? text
                : text.substr(0, first_line_end);

        return first_line == protocol;
    }

    int base64_value(char value) {
        if (value >= 'A' && value <= 'Z') {
            return value - 'A';
        }
        if (value >= 'a' && value <= 'z') {
            return value - 'a' + 26;
        }
        if (value >= '0' && value <= '9') {
            return value - '0' + 52;
        }
        if (value == '+') {
            return 62;
        }
        if (value == '/') {
            return 63;
        }
        return -1;
    }

    bool decode_base64(
        const std::string& encoded,
        std::string& decoded
    ) {
        decoded.clear();

        unsigned long accumulator = 0;
        int bit_count = 0;

        for (
            std::string::size_type index = 0;
            index < encoded.size();
            ++index
        ) {
            char character = encoded[index];

            if (character == '=') {
                break;
            }

            int value = base64_value(character);
            if (value < 0) {
                decoded.clear();
                return false;
            }

            accumulator =
                (accumulator << 6) |
                (unsigned long)value;
            bit_count += 6;

            if (bit_count >= 8) {
                bit_count -= 8;
                char byte_value = (char)(
                    (accumulator >> bit_count) & 0xFF
                );
                decoded.append(1, byte_value);

                if (bit_count == 0) {
                    accumulator = 0;
                } else {
                    accumulator &=
                        (1UL << bit_count) - 1UL;
                }
            }
        }

        return true;
    }

    bool apply_attachment_payload(
        const std::string& payload,
        ConversationEvent& event
    ) {
        if (
            !has_exact_protocol_line(
                payload,
                attachment_protocol
            )
        ) {
            return false;
        }

        std::string encoded_name =
            get_protocol_value(payload, "name_base64");
        std::string mime_type =
            get_protocol_value(payload, "mime_type");
        std::string encoded_data =
            get_protocol_value(payload, "data_base64");
        std::string size_text =
            get_protocol_value(payload, "size");

        if (
            encoded_name.empty() ||
            size_text.empty()
        ) {
            return false;
        }

        std::string name;
        std::string data;

        if (
            !decode_base64(encoded_name, name) ||
            !decode_base64(encoded_data, data)
        ) {
            return false;
        }

        char* size_end = 0;
        unsigned long expected_size =
            strtoul(size_text.c_str(), &size_end, 10);

        if (
            size_end == size_text.c_str() ||
            size_end == 0 ||
            *size_end != '\0' ||
            expected_size != (unsigned long)data.size() ||
            expected_size > ConversationAttachmentPolicy::maximum_attachment_bytes()
        ) {
            return false;
        }

        event.set_attachment(
            name.c_str(),
            mime_type.empty()
                ? "application/octet-stream"
                : mime_type.c_str(),
            data
        );
        return true;
    }

    void append_timing_part(
        std::string& output,
        const std::string& metadata,
        const char* key,
        const char* label
    ) {
        std::string value = get_protocol_value(metadata, key);

        if (value.empty()) {
            return;
        }

        unsigned long milliseconds =
            strtoul(value.c_str(), 0, 10);

        char part[96];
        sprintf(
            part,
            "%s%s %lu ms",
            output.empty() ? "" : " | ",
            label,
            milliseconds
        );
        output += part;
    }

    void build_relay_timing_text(
        const std::string& metadata,
        std::string& output
    ) {
        output.clear();

        append_timing_part(
            output,
            metadata,
            "timing_bridge_total_ms",
            "bridge"
        );
        append_timing_part(
            output,
            metadata,
            "timing_broker_total_ms",
            "broker"
        );
        append_timing_part(
            output,
            metadata,
            "timing_broker_queue_ms",
            "queue"
        );
        append_timing_part(
            output,
            metadata,
            "timing_background_total_ms",
            "extension"
        );
        append_timing_part(
            output,
            metadata,
            "timing_browser_total_ms",
            "browser"
        );
        append_timing_part(
            output,
            metadata,
            "timing_browser_submit_ms",
            "submit"
        );
        append_timing_part(
            output,
            metadata,
            "timing_browser_first_response_ms",
            "first response"
        );
        append_timing_part(
            output,
            metadata,
            "timing_browser_generation_ms",
            "generation"
        );
        append_timing_part(
            output,
            metadata,
            "timing_browser_stabilization_ms",
            "stabilize"
        );

        if (!output.empty()) {
            output.insert(0, "Relay timing: ");
        }
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
        if (name == "attachment") {
            return ConversationEvent::event_attachment;
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
        std::vector<ConversationEvent>& output,
        std::string& timing_text
    ) {
        output.clear();
        timing_text.clear();

        std::string::size_type header_end = payload.find("\n\n");
        if (header_end == std::string::npos) {
            return false;
        }

        std::string metadata = payload.substr(0, header_end);
        if (!has_exact_protocol_line(metadata, conversation_protocol)) {
            return false;
        }

        if (get_protocol_value(metadata, "status") != "ok") {
            return false;
        }

        std::string mode =
            get_protocol_value(metadata, "mode");

        if (mode == "probe") {
            if (
                get_protocol_value(metadata, "text_forwarded") != "0" ||
                get_protocol_value(metadata, "attachments_forwarded") != "0" ||
                get_protocol_value(metadata, "credentials_forwarded") != "0" ||
                get_protocol_value(metadata, "session_forwarded") != "0" ||
                get_protocol_value(metadata, "transport_security") != "plaintext"
            ) {
                return false;
            }
        } else if (mode == "browser_relay") {
            std::string text_forwarded =
                get_protocol_value(metadata, "text_forwarded");
            std::string attachments_forwarded =
                get_protocol_value(metadata, "attachments_forwarded");

            if (
                (text_forwarded != "0" && text_forwarded != "1") ||
                (attachments_forwarded != "0" &&
                 attachments_forwarded != "1") ||
                (text_forwarded == "0" &&
                 attachments_forwarded == "0") ||
                get_protocol_value(metadata, "credentials_forwarded") != "0" ||
                get_protocol_value(metadata, "session_forwarded") != "0" ||
                get_protocol_value(metadata, "transport_security") != "trusted_lan"
            ) {
                return false;
            }
        } else {
            return false;
        }

        if (mode == "browser_relay") {
            build_relay_timing_text(
                metadata,
                timing_text
            );
        }

        unsigned long request_id =
            get_protocol_unsigned(metadata, "request_id");

        if (request_id == 0 || request_id != expected_request_id) {
            return false;
        }

        unsigned long event_count =
            get_protocol_unsigned(metadata, "event_count");

        if (event_count == 0 || event_count > 64) {
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

            sprintf(type_key, "event_%lu_type", index);
            sprintf(length_key, "event_%lu_len", index);

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

            if (type == ConversationEvent::event_attachment) {
                if (!apply_attachment_payload(
                        event_text,
                        event
                    )) {
                    return false;
                }
            } else {
                event.set_text(event_text.c_str());
            }

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
    pending_operation(operation_none),
    capability_state(capability_unknown),
    active_request_id(0),
    status_text("stopped") {
}

const char* RemoteConversationBackend::get_name() const {
    return "Remote Conversation Bridge Backend";
}

const char* RemoteConversationBackend::get_status_text() const {
    return status_text.c_str();
}

const char* RemoteConversationBackend::get_diagnostic_text() const {
    return diagnostic_text.c_str();
}

void RemoteConversationBackend::get_security_profile(
    ConversationSecurityProfile& profile
) const {
    profile = ConversationSecurityProfile();
    profile.dispatch_mode = conversation_dispatch_content;
    profile.transport_security =
        conversation_transport_trusted_lan;
    profile.text = true;
    profile.attachments = true;
    profile.credentials = false;
    profile.session_state = false;
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
        status_text = "bridge configuration incomplete";
        return false;
    }

    if (!request_executor->initialize()) {
        status_text = "network executor initialization failed";
        return false;
    }

    bridge_online = false;
    pending_operation = operation_none;
    capability_state = capability_unknown;
    active_request_id = 0;
    diagnostic_text.clear();
    events.clear();
    is_initialized = true;

    if (!begin_health_check()) {
        set_capability_status(
            capability_unreachable,
            "companion capability check could not start"
        );
    }

    return true;
}

void RemoteConversationBackend::update() {
    if (
        !is_initialized ||
        request_executor == 0 ||
        pending_operation == operation_none
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

    PendingOperation completed_operation = pending_operation;
    pending_operation = operation_none;

    if (!succeeded) {
        bridge_online = false;

        if (completed_operation == operation_health) {
            std::string status(
                "companion capability check failed"
            );

            if (!error_text.empty()) {
                status += ": ";
                status += error_text;
            }

            set_capability_status(
                capability_unreachable,
                status.c_str()
            );
            return;
        }

        queue_failure(
            active_request_id,
            error_text.empty()
                ? "Remote conversation browser relay transport failed."
                : error_text.c_str()
        );
        active_request_id = 0;
        return;
    }

    bridge_online = true;

    if (completed_operation == operation_health) {
        apply_health_response(response);
        return;
    }

    if (completed_operation == operation_conversation) {
        if (!parse_response(response, active_request_id)) {
            active_request_id = 0;
            return;
        }

        active_request_id = 0;
    }
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
    pending_operation = operation_none;
    capability_state = capability_unknown;
    active_request_id = 0;
    status_text = "stopped";
    diagnostic_text.clear();
    events.clear();
}

bool RemoteConversationBackend::get_is_initialized() const {
    return is_initialized;
}

bool RemoteConversationBackend::submit_probe(
    unsigned long request_id
) {
    if (
        !is_initialized ||
        request_executor == 0 ||
        request_id == 0 ||
        pending_operation != operation_none ||
        !events.empty() ||
        request_executor->get_is_busy()
    ) {
        return false;
    }

    if (capability_state != capability_ready) {
        if (
            capability_state == capability_incompatible ||
            capability_state == capability_unreachable ||
            capability_state == capability_unknown
        ) {
            begin_health_check();
        }

        return false;
    }

    // SECURITY: This method is deliberately content-free.
    // ConversationServiceHost does not pass ConversationRequest to a
    // probe-only backend. Only the request ID and fixed zero-forwarding
    // flags can cross the plaintext LAN through this path.
    char body[320];
    sprintf(
        body,
        "%s\n"
        "mode=probe\n"
        "request_id=%lu\n"
        "text_forwarded=0\n"
        "attachments_forwarded=0\n"
        "credentials_forwarded=0\n"
        "session_forwarded=0\n",
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
        status_text = "conversation probe could not start";
        return false;
    }

    active_request_id = request_id;
    pending_operation = operation_conversation;
    status_text = "SALIX-CONVERSATION/1 request in flight";
    return true;
}

bool RemoteConversationBackend::submit_request(
    const ConversationRequest& request,
    unsigned long request_id
) {
    if (
        !is_initialized ||
        request_executor == 0 ||
        request_id == 0 ||
        request.empty() ||
        pending_operation != operation_none ||
        !events.empty() ||
        request_executor->get_is_busy()
    ) {
        return false;
    }

    if (capability_state != capability_ready) {
        if (
            capability_state == capability_incompatible ||
            capability_state == capability_unreachable ||
            capability_state == capability_unknown
        ) {
            begin_health_check();
        }

        return false;
    }

    std::string text(request.get_text());
    int attachment_count = request.get_attachment_count();

    if (
        attachment_count < 0 ||
        (unsigned long)attachment_count > ConversationAttachmentPolicy::maximum_attachment_count
    ) {
        char limit_text[96];
        sprintf(
            limit_text,
            "browser relay supports at most %d attachments",
            ConversationAttachmentPolicy::maximum_attachment_count
        );
        status_text = limit_text;
        return false;
    }

    if (text.empty() && attachment_count == 0) {
        return false;
    }

    std::vector<OutgoingAttachment> attachments;
    unsigned long total_attachment_bytes = 0;

    for (int index = 0; index < attachment_count; ++index) {
        const char* path = request.get_attachment_path(index);

        OutgoingAttachment attachment;
        attachment.name = get_file_name(path);

        if (attachment.name.empty()) {
            status_text = "attachment filename is invalid";
            return false;
        }

        attachment.mime_type =
            get_attachment_mime_type(attachment.name);

        if (!read_attachment_file(path, attachment.data)) {
            char limit_text[128];
            sprintf(
                limit_text,
                "attachment could not be read or exceeds %d MB",
                ConversationAttachmentPolicy::
                    maximum_attachment_megabytes
            );
            status_text = limit_text;
            return false;
        }

        total_attachment_bytes +=
            (unsigned long)attachment.data.size();

        if (
            total_attachment_bytes >
            ConversationAttachmentPolicy::maximum_total_attachment_bytes()
        ) {
            char limit_text[128];
            sprintf(
                limit_text,
                "attachments exceed %d MB total relay limit",
                ConversationAttachmentPolicy::
                    maximum_total_attachment_megabytes
            );
            status_text = limit_text;
            return false;
        }

        attachments.push_back(attachment);
    }

    std::string metadata;
    char line[256];

    metadata += conversation_protocol;
    metadata += "\nmode=browser_relay\n";

    sprintf(line, "request_id=%lu\n", request_id);
    metadata += line;

    metadata += text.empty()
        ? "text_forwarded=0\n"
        : "text_forwarded=1\n";
    metadata += attachments.empty()
        ? "attachments_forwarded=0\n"
        : "attachments_forwarded=1\n";
    metadata +=
        "credentials_forwarded=0\n"
        "session_forwarded=0\n";

    sprintf(
        line,
        "text_len=%lu\nattachment_count=%lu\n",
        (unsigned long)text.size(),
        (unsigned long)attachments.size()
    );
    metadata += line;

    for (
        unsigned long index = 0;
        index < (unsigned long)attachments.size();
        ++index
    ) {
        const OutgoingAttachment& attachment =
            attachments[index];

        sprintf(
            line,
            "attachment_%lu_name_len=%lu\n"
            "attachment_%lu_mime_len=%lu\n"
            "attachment_%lu_data_len=%lu\n",
            index,
            (unsigned long)attachment.name.size(),
            index,
            (unsigned long)attachment.mime_type.size(),
            index,
            (unsigned long)attachment.data.size()
        );
        metadata += line;
    }

    metadata += "\n";

    std::string body(metadata);
    body += text;

    for (
        unsigned long index = 0;
        index < (unsigned long)attachments.size();
        ++index
    ) {
        body += attachments[index].name;
        body += attachments[index].mime_type;
        body += attachments[index].data;
    }

    NetworkRequest network_request(
        "POST",
        "/v1/conversation/message"
    );
    network_request.set_content_type(
        "application/x-salix-conversation"
    );
    network_request.set_body(body);

    if (!request_executor->submit(
            host.c_str(),
            port,
            network_request
        )) {
        status_text = "browser relay request could not start";
        return false;
    }

    active_request_id = request_id;
    pending_operation = operation_conversation;
    status_text =
        "SALIX-CONVERSATION/1 browser relay request in flight";
    return true;
}

bool RemoteConversationBackend::take_event(
    ConversationEvent& event
) {
    event.clear();

    if (events.empty()) {
        return false;
    }

    // Drain every semantic event that is already available. The completed
    // browser-relay response currently arrives as one bounded event queue, so
    // throttling to one event per runtime update only stretches presentation
    // across many frames. Future true streaming remains incremental because
    // only events actually queued by that update can be drained.
    event = events[0];
    events.erase(events.begin());

    if (
        events.empty() &&
        pending_operation == operation_none &&
        capability_state == capability_ready
    ) {
        status_text =
            "SALIX-CONVERSATION/1 ready | browser relay | trusted LAN | text + files";
    }

    return true;
}

bool RemoteConversationBackend::begin_health_check() {
    if (
        !is_initialized ||
        request_executor == 0 ||
        pending_operation != operation_none ||
        request_executor->get_is_busy()
    ) {
        return false;
    }

    NetworkRequest health_request("GET", "/v1/health");
    health_request.set_content_type("text/plain");

    if (!request_executor->submit(
            host.c_str(),
            port,
            health_request
        )) {
        return false;
    }

    pending_operation = operation_health;
    set_capability_status(
        capability_checking,
        "checking companion conversation capability"
    );
    return true;
}

void RemoteConversationBackend::apply_health_response(
    const NetworkResponse& response
) {
    if (!response.get_is_success()) {
        char status[192];
        sprintf(
            status,
            "companion capability check returned HTTP %d",
            response.get_status_code()
        );
        set_capability_status(
            capability_incompatible,
            status
        );
        return;
    }

    const std::string& body = response.get_body();

    if (
        !has_exact_protocol_line(body, bridge_protocol) ||
        get_protocol_value(body, "status") != "ok"
    ) {
        set_capability_status(
            capability_incompatible,
            "companion health response is not SALIX-BRIDGE/1"
        );
        return;
    }

    if (
        get_protocol_value(body, "conversation_probe") !=
        "enabled"
    ) {
        set_capability_status(
            capability_incompatible,
            "companion lacks conversation probe; update/restart companion"
        );
        return;
    }

    if (
        get_protocol_value(body, "conversation_protocol") !=
        conversation_protocol
    ) {
        set_capability_status(
            capability_incompatible,
            "conversation protocol mismatch; update/restart companion"
        );
        return;
    }

    if (
        get_protocol_value(body, "conversation_relay") !=
            "enabled" ||
        get_protocol_value(body, "conversation_mode") !=
            "browser_relay" ||
        get_protocol_value(
            body,
            "conversation_text_forwarding"
        ) != "enabled" ||
        get_protocol_value(
            body,
            "conversation_attachment_forwarding"
        ) != "enabled" ||
        get_protocol_value(
            body,
            "conversation_credential_forwarding"
        ) != "disabled" ||
        get_protocol_value(
            body,
            "conversation_session_forwarding"
        ) != "disabled" ||
        get_protocol_value(
            body,
            "conversation_transport_security"
        ) != "trusted_lan"
    ) {
        set_capability_status(
            capability_incompatible,
            "conversation browser-relay policy mismatch; update/restart companion"
        );
        return;
    }

    std::string browser_session =
        get_protocol_value(
            body,
            "conversation_browser_session"
        );

    if (browser_session != "ready") {
        std::string status(
            "browser relay not ready: "
        );
        status += browser_session.empty()
            ? "start tools\\salix_chat_session.py"
            : browser_session;

        set_capability_status(
            capability_unreachable,
            status.c_str()
        );
        return;
    }

    set_capability_status(
        capability_ready,
        "SALIX-CONVERSATION/1 ready | browser relay | trusted LAN | text only"
    );
}

bool RemoteConversationBackend::parse_response(
    const NetworkResponse& response,
    unsigned long expected_request_id
) {
    if (!response.get_is_success()) {
        char detail[256];

        if (response.get_status_code() == 404) {
            sprintf(
                detail,
                "Conversation browser-relay endpoint is unavailable (HTTP 404). "
                "Update/restart tools\\salix_bridge.py on the companion."
            );
            set_capability_status(
                capability_incompatible,
                "companion browser-relay endpoint missing; update/restart companion"
            );
        } else {
            sprintf(
                detail,
                "Remote conversation browser relay returned HTTP %d.",
                response.get_status_code()
            );
        }

        queue_failure(expected_request_id, detail);
        return false;
    }

    std::vector<ConversationEvent> parsed_events;
    std::string parsed_timing_text;

    if (!parse_conversation_payload(
            response.get_body(),
            expected_request_id,
            parsed_events,
            parsed_timing_text
        )) {
        queue_failure(
            expected_request_id,
            "Remote conversation browser relay returned an invalid semantic payload."
        );
        return false;
    }

    events.insert(
        events.end(),
        parsed_events.begin(),
        parsed_events.end()
    );
    diagnostic_text = parsed_timing_text;
    status_text = "SALIX-CONVERSATION/1 events received";
    return true;
}

void RemoteConversationBackend::set_capability_status(
    CapabilityState state,
    const char* text
) {
    capability_state = state;
    status_text =
        text == 0 || text[0] == '\0'
            ? "conversation capability status unavailable"
            : text;
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
            ? "Remote conversation request failed."
            : detail
    );
    events.push_back(event);
}
