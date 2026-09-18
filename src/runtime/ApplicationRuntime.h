// =================================================================================
// Filename:    runtime/ApplicationRuntime.h
// Author:      Ebdsaleh
// Description: Declares the application runtime lifecycle coordinator.
// =================================================================================
#pragma once

#include "ServiceRegistry.h"
#include "RuntimeStatusService.h"

class ConversationServiceHost;
class WebPlatformHost;

class ApplicationRuntime {
    public:
        ApplicationRuntime();

        bool set_web_platform_host(WebPlatformHost* host);
        WebPlatformHost* get_web_platform_host();
        const WebPlatformHost* get_web_platform_host() const;

        bool set_conversation_service_host(ConversationServiceHost* host);
        ConversationServiceHost* get_conversation_service_host();
        const ConversationServiceHost* get_conversation_service_host() const;

        bool initialize();
        void update();
        void shutdown();

        bool get_is_initialized() const;
        int get_service_count() const;
        unsigned long get_update_count() const;
        ServiceRegistry& get_services();

    private:
        bool register_core_services();

        ServiceRegistry service_registry;
        RuntimeStatusService runtime_status_service;
        WebPlatformHost* web_platform_host;
        ConversationServiceHost* conversation_service_host;
        bool core_services_registered;
        bool is_initialized;
};
