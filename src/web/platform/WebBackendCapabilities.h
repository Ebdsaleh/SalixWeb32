// =================================================================================
// Filename:    web/platform/WebBackendCapabilities.h
// Author:      Ebdsaleh
// Description: Declares backend-family and capability discovery primitives.
// =================================================================================
#pragma once

enum WebBackendFamily {
    web_backend_placeholder = 0,
    web_backend_native,
    web_backend_gecko,
    web_backend_translator,
    web_backend_remote
};

const char* get_web_backend_family_name(WebBackendFamily family);

class WebBackendCapabilities {
    public:
        WebBackendCapabilities();

        bool navigation;
        bool surface_snapshot;
        bool pointer_input;
        bool keyboard_input;
        bool network;
        bool html;
        bool css;
        bool javascript;
        bool websocket;
        bool file_upload;
};
