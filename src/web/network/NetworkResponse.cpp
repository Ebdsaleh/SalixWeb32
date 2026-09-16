// =================================================================================
// Filename:    web/network/NetworkResponse.cpp
// Author:      Ebdsaleh
// Description: Implements the small backend-neutral HTTP response model.
// =================================================================================

#include "NetworkResponse.h"

NetworkResponse::NetworkResponse()
    : status_code(0) {
}

void NetworkResponse::clear() {
    status_code = 0;
    status_text.clear();
    body.clear();
}

void NetworkResponse::set_status_code(int new_status_code) {
    status_code = new_status_code;
}

int NetworkResponse::get_status_code() const {
    return status_code;
}

void NetworkResponse::set_status_text(const char* new_status_text) {
    status_text = new_status_text == 0 ? "" : new_status_text;
}

const std::string& NetworkResponse::get_status_text() const {
    return status_text;
}

void NetworkResponse::set_body(const std::string& new_body) {
    body = new_body;
}

const std::string& NetworkResponse::get_body() const {
    return body;
}

bool NetworkResponse::get_is_success() const {
    return status_code >= 200 && status_code < 300;
}
