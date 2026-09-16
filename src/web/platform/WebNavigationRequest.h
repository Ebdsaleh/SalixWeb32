// =================================================================================
// Filename:    web/platform/WebNavigationRequest.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral web navigation request.
// =================================================================================
#pragma once

#include <string>

class WebNavigationRequest {
    public:
        enum Kind {
            navigation_typed = 0,
            navigation_link,
            navigation_reload
        };

        WebNavigationRequest();
        WebNavigationRequest(const char* url);

        void set_url(const char* url);
        const char* get_url() const;

        void set_kind(Kind kind);
        Kind get_kind() const;

        void set_replace_history(bool replace_history);
        bool get_replace_history() const;

        bool empty() const;

    private:
        std::string url;
        Kind kind;
        bool replace_history;
};
