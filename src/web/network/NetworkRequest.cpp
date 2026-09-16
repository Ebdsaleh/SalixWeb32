// =================================================================================
// Filename:    web/network/NetworkRequest.cpp
// Author:      Ebdsaleh
// Description: Implements the small backend-neutral HTTP request model.
// =================================================================================

#include "NetworkRequest.h"

NetworkRequest::NetworkRequest()
    : method("GET"),
      path("/"),
      content_type("text/plain") {
}

NetworkRequest::NetworkRequest(const char* new_method, const char* new_path)
    : method(new_method == 0 ? "GET" : new_method),
      path(new_path == 0 || new_path[0] == '\0' ? "/" : new_path),
      content_type("text/plain") {
}

void NetworkRequest::set_method(const char* new_method) {
    method = new_method == 0 ? "GET" : new_method;
}

const std::string& NetworkRequest::get_method() const {
    return method;
}

void NetworkRequest::set_path(const char* new_path) {
    path = new_path == 0 || new_path[0] == '\0' ? "/" : new_path;
}

const std::string& NetworkRequest::get_path() const {
    return path;
}

void NetworkRequest::set_content_type(const char* new_content_type) {
    content_type = new_content_type == 0 ? "" : new_content_type;
}

const std::string& NetworkRequest::get_content_type() const {
    return content_type;
}

void NetworkRequest::set_body(const char* new_body) {
    body = new_body == 0 ? "" : new_body;
}

void NetworkRequest::set_body(const std::string& new_body) {
    body = new_body;
}

const std::string& NetworkRequest::get_body() const {
    return body;
}
