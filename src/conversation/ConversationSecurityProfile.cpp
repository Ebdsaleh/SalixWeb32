// =================================================================================
// Filename:    conversation/ConversationSecurityProfile.cpp
// Author:      Ebdsaleh
// Description: Implements conversation content/security boundary checks.
// =================================================================================

#include "ConversationSecurityProfile.h"
#include "ConversationRequest.h"

const char* get_conversation_dispatch_mode_name(
    ConversationDispatchMode mode
) {
    switch (mode) {
        case conversation_dispatch_probe_only:
            return "probe-only";
        case conversation_dispatch_content:
            return "content";
        case conversation_dispatch_blocked:
        default:
            return "blocked";
    }
}

const char* get_conversation_transport_security_name(
    ConversationTransportSecurity security
) {
    switch (security) {
        case conversation_transport_local_process:
            return "local-process";
        case conversation_transport_plaintext:
            return "plaintext";
        case conversation_transport_authenticated_encrypted:
            return "authenticated-encrypted";
        case conversation_transport_none:
        default:
            return "none";
    }
}

ConversationSecurityProfile::ConversationSecurityProfile()
    : dispatch_mode(conversation_dispatch_blocked),
      transport_security(conversation_transport_none),
      text(false),
      attachments(false),
      credentials(false),
      session_state(false) {
}

bool ConversationSecurityProfile::allows_content_transport() const {
    return
        dispatch_mode == conversation_dispatch_content &&
        (
            transport_security == conversation_transport_local_process ||
            transport_security ==
                conversation_transport_authenticated_encrypted
        );
}

bool ConversationSecurityProfile::allows_content_request(
    const ConversationRequest& request
) const {
    if (!allows_content_transport()) {
        return false;
    }

    if (request.get_text()[0] != '\0' && !text) {
        return false;
    }

    if (
        request.get_attachment_count() > 0 &&
        !attachments
    ) {
        return false;
    }

    return true;
}
