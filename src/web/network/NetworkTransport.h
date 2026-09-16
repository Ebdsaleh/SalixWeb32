// =================================================================================
// Filename:    web/network/NetworkTransport.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral request transport contract.
// =================================================================================
#pragma once

class NetworkRequest;
class NetworkResponse;

class NetworkTransport {
    public:
        virtual ~NetworkTransport() {}

        virtual bool initialize() = 0;
        virtual void shutdown() = 0;
        virtual bool get_is_initialized() const = 0;

        virtual bool send(
            const char* host,
            unsigned short port,
            const NetworkRequest& request,
            NetworkResponse& response
        ) = 0;

        virtual const char* get_last_error() const = 0;
};
