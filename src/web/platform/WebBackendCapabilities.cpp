// =================================================================================
// Filename:    web/platform/WebBackendCapabilities.cpp
// Author:      Ebdsaleh
// Description: Implements backend-family and capability discovery primitives.
// =================================================================================

#include "WebBackendCapabilities.h"

const char* get_web_backend_family_name(WebBackendFamily family) {
    switch (family) {
        case web_backend_native:
            return "native";
        case web_backend_gecko:
            return "gecko";
        case web_backend_translator:
            return "translator";
        case web_backend_remote:
            return "remote";
        case web_backend_placeholder:
        default:
            return "placeholder";
    }
}

WebBackendCapabilities::WebBackendCapabilities()
    : navigation(false),
      surface_snapshot(false),
      pointer_input(false),
      keyboard_input(false),
      network(false),
      html(false),
      css(false),
      javascript(false),
      websocket(false),
      file_upload(false) {
}
