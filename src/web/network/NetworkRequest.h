// =================================================================================
// Filename:    web/network/NetworkRequest.h
// Author:      Ebdsaleh
// Description: Declares a small backend-neutral HTTP request model.
// =================================================================================
#pragma once

#include <string>

class NetworkRequest {
    public:
        NetworkRequest();
        NetworkRequest(const char* method, const char* path);

        void set_method(const char* method);
        const std::string& get_method() const;

        void set_path(const char* path);
        const std::string& get_path() const;

        void set_content_type(const char* content_type);
        const std::string& get_content_type() const;

        void set_body(const char* body);
        void set_body(const std::string& body);
        const std::string& get_body() const;

    private:
        std::string method;
        std::string path;
        std::string content_type;
        std::string body;
};
