// =================================================================================
// Filename:    conversation/ConversationSecurityProfile.h
// Author:      Ebdsaleh
// Description: Declares machine-readable conversation content/security boundaries.
// =================================================================================
#pragma once

class ConversationRequest;

enum ConversationDispatchMode {
    conversation_dispatch_blocked = 0,
    conversation_dispatch_probe_only,
    conversation_dispatch_content
};

enum ConversationTransportSecurity {
    conversation_transport_none = 0,
    conversation_transport_local_process,
    conversation_transport_plaintext,
    conversation_transport_trusted_lan,
    conversation_transport_authenticated_encrypted
};

const char* get_conversation_dispatch_mode_name(
    ConversationDispatchMode mode
);

const char* get_conversation_transport_security_name(
    ConversationTransportSecurity security
);

class ConversationSecurityProfile {
    public:
        ConversationSecurityProfile();

        bool allows_content_request(
            const ConversationRequest& request
        ) const;

        bool allows_content_transport() const;

        ConversationDispatchMode dispatch_mode;
        ConversationTransportSecurity transport_security;

        bool text;
        bool attachments;
        bool credentials;
        bool session_state;
};
