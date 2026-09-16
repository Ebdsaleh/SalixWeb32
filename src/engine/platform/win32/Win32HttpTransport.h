// =================================================================================
// Filename:    engine/platform/win32/Win32HttpTransport.h
// Author:      Ebdsaleh
// Description: Declares the legacy-compatible plain HTTP Winsock transport.
// =================================================================================
#pragma once

#include <string>

#include "web/network/NetworkTransport.h"

class NetworkRequest;
class NetworkResponse;

class Win32HttpTransport : public NetworkTransport {
    public:
        Win32HttpTransport();
        virtual ~Win32HttpTransport();

        virtual bool initialize();
        virtual void shutdown();
        virtual bool get_is_initialized() const;

        virtual bool send(
            const char* host,
            unsigned short port,
            const NetworkRequest& request,
            NetworkResponse& response
        );

        virtual const char* get_last_error() const;

        void set_timeout_milliseconds(int timeout_milliseconds);

    private:
        void set_error(const char* text);

        bool is_initialized;
        int timeout_milliseconds;
        std::string last_error;
};
