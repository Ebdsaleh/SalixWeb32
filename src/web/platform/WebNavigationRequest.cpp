// =================================================================================
// Filename:    web/platform/WebNavigationRequest.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral web navigation request.
// =================================================================================

#include "WebNavigationRequest.h"

WebNavigationRequest::WebNavigationRequest()
    : kind(navigation_typed),
      replace_history(false) {
}

WebNavigationRequest::WebNavigationRequest(const char* new_url)
    : kind(navigation_typed),
      replace_history(false) {
    set_url(new_url);
}

void WebNavigationRequest::set_url(const char* new_url) {
    url = new_url == 0 ? "" : new_url;
}

const char* WebNavigationRequest::get_url() const {
    return url.c_str();
}

void WebNavigationRequest::set_kind(Kind new_kind) {
    kind = new_kind;
}

WebNavigationRequest::Kind WebNavigationRequest::get_kind() const {
    return kind;
}

void WebNavigationRequest::set_replace_history(bool new_replace_history) {
    replace_history = new_replace_history;
}

bool WebNavigationRequest::get_replace_history() const {
    return replace_history;
}

bool WebNavigationRequest::empty() const {
    return url.empty();
}
