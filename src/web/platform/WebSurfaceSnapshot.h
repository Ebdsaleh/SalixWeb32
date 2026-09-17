// =================================================================================
// Filename:    web/platform/WebSurfaceSnapshot.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral WebView surface/probe snapshot.
// =================================================================================
#pragma once

#include <string>

class WebSurfaceSnapshot {
    public:
        WebSurfaceSnapshot();

        void clear();

        std::string title;
        std::string address;
        std::string status;
        std::string content;

        // Optional Browser Probe metadata. Backends that do not perform an
        // HTTP/document probe leave these fields at their cleared defaults.
        int http_status_code;
        std::string http_status_text;
        std::string final_address;
        std::string mime_type;
        unsigned long response_size;
        bool response_truncated;
        int redirect_count;
        int script_count;
        int form_count;
        int link_count;
        std::string response_headers;
        std::string raw_content;
        std::string extracted_content;
};
