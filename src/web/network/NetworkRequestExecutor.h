// =================================================================================
// Filename:    web/network/NetworkRequestExecutor.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral asynchronous network request boundary.
// =================================================================================
#pragma once

#include <string>

class NetworkRequest;
class NetworkResponse;

class NetworkRequestExecutor {
    public:
        virtual ~NetworkRequestExecutor() {}

        virtual bool initialize() = 0;
        virtual void shutdown() = 0;
        virtual bool get_is_initialized() const = 0;

        virtual bool submit(
            const char* host,
            unsigned short port,
            const NetworkRequest& request
        ) = 0;

        virtual bool get_is_busy() const = 0;

        // Completion is consumed explicitly by the application's update thread.
        // A false return means no completed request is waiting.
        virtual bool take_result(
            NetworkResponse& response,
            std::string& error_text,
            bool& succeeded
        ) = 0;

        virtual const char* get_last_error() const = 0;
};
