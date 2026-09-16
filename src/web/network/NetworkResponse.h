// =================================================================================
// Filename:    web/network/NetworkResponse.h
// Author:      Ebdsaleh
// Description: Declares a small backend-neutral HTTP response model.
// =================================================================================
#pragma once

#include <string>

class NetworkResponse {
    public:
        NetworkResponse();

        void clear();

        void set_status_code(int status_code);
        int get_status_code() const;

        void set_status_text(const char* status_text);
        const std::string& get_status_text() const;

        void set_body(const std::string& body);
        const std::string& get_body() const;

        bool get_is_success() const;

    private:
        int status_code;
        std::string status_text;
        std::string body;
};
