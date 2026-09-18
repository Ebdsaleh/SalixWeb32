// =================================================================================
// Filename:    engine/platform/win32/Win32SecureTransportProvider.cpp
// Author:      Ebdsaleh
// Description: Implements fail-closed secure transport provider discovery.
// =================================================================================

#include <string.h>

#include "Win32SecureTransportProvider.h"

namespace {
    const char* provider_dll_name =
        "SalixSecureTransport.dll";

    bool has_required_security_capabilities(
        unsigned long capabilities
    ) {
        const unsigned long tls_mask =
            SALIX_SECURE_TRANSPORT_CAP_TLS_1_2 |
            SALIX_SECURE_TRANSPORT_CAP_TLS_1_3;

        if ((capabilities & tls_mask) == 0) {
            return false;
        }

        if (
            (capabilities &
             SALIX_SECURE_TRANSPORT_CAP_PEER_AUTHENTICATION) == 0
        ) {
            return false;
        }

        if (
            (capabilities &
             SALIX_SECURE_TRANSPORT_CAP_CERTIFICATE_PINNING) == 0
        ) {
            return false;
        }

        return true;
    }
}

Win32SecureTransportProvider::Win32SecureTransportProvider()
    : module_handle(NULL),
      is_ready(false),
      capabilities(0),
      provider_name("Salix Secure Transport Provider"),
      status_text("not checked | real content remains blocked") {
}

Win32SecureTransportProvider::~Win32SecureTransportProvider() {
    shutdown();
}

void Win32SecureTransportProvider::initialize(
    const char* executable_directory
) {
    shutdown();

    if (
        executable_directory == 0 ||
        executable_directory[0] == '\0'
    ) {
        set_unavailable(
            "executable directory unavailable | real content remains blocked"
        );
        return;
    }

    std::string module_path(executable_directory);

    if (
        !module_path.empty() &&
        module_path[module_path.size() - 1] != '\\' &&
        module_path[module_path.size() - 1] != '/'
    ) {
        module_path += "\\";
    }

    module_path += provider_dll_name;

    if (module_path.size() >= MAX_PATH) {
        set_unavailable(
            "provider path too long | real content remains blocked"
        );
        return;
    }

    DWORD attributes =
        GetFileAttributesA(module_path.c_str());

    if (
        attributes == INVALID_FILE_ATTRIBUTES ||
        (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0
    ) {
        set_unavailable(
            "not installed | real content remains blocked"
        );
        return;
    }

    // Use an absolute executable-directory path. Never ask Windows to
    // resolve SalixSecureTransport.dll from the process search path.
    module_handle = LoadLibraryA(module_path.c_str());

    if (module_handle == NULL) {
        set_incompatible(
            "present but could not load | real content remains blocked"
        );
        return;
    }

    FARPROC abi_version_address = GetProcAddress(
        module_handle,
        "salix_secure_transport_get_abi_version"
    );
    FARPROC capabilities_address = GetProcAddress(
        module_handle,
        "salix_secure_transport_get_capabilities"
    );
    FARPROC provider_name_address = GetProcAddress(
        module_handle,
        "salix_secure_transport_get_provider_name"
    );

    if (
        abi_version_address == 0 ||
        capabilities_address == 0 ||
        provider_name_address == 0
    ) {
        set_incompatible(
            "missing required ABI exports | real content remains blocked"
        );
        return;
    }

    SalixSecureTransportGetAbiVersionFunction get_abi_version = 0;
    SalixSecureTransportGetCapabilitiesFunction get_capabilities = 0;
    SalixSecureTransportGetProviderNameFunction get_provider_name = 0;

    // Avoid the MSVC C4191 FARPROC conversion warning. Win32 function
    // pointers are the same size here; memcpy preserves the raw address
    // without crossing a C++ ABI boundary.
    if (
        sizeof(get_abi_version) != sizeof(abi_version_address) ||
        sizeof(get_capabilities) != sizeof(capabilities_address) ||
        sizeof(get_provider_name) != sizeof(provider_name_address)
    ) {
        set_incompatible(
            "function pointer ABI size mismatch | real content remains blocked"
        );
        return;
    }

    memcpy(
        &get_abi_version,
        &abi_version_address,
        sizeof(get_abi_version)
    );
    memcpy(
        &get_capabilities,
        &capabilities_address,
        sizeof(get_capabilities)
    );
    memcpy(
        &get_provider_name,
        &provider_name_address,
        sizeof(get_provider_name)
    );

    if (
        get_abi_version == 0 ||
        get_capabilities == 0 ||
        get_provider_name == 0
    ) {
        set_incompatible(
            "missing required ABI exports | real content remains blocked"
        );
        return;
    }

    if (
        get_abi_version() !=
        SALIX_SECURE_TRANSPORT_ABI_VERSION
    ) {
        set_incompatible(
            "ABI version mismatch | real content remains blocked"
        );
        return;
    }

    const char* reported_name = get_provider_name();

    if (
        reported_name != 0 &&
        reported_name[0] != '\0'
    ) {
        provider_name = reported_name;
    }

    unsigned long reported_capabilities =
        get_capabilities();

    if (
        !has_required_security_capabilities(
            reported_capabilities
        )
    ) {
        set_incompatible(
            "ABI 1 loaded | required TLS/authentication/pinning capabilities missing | real content remains blocked"
        );
        return;
    }

    capabilities = reported_capabilities;
    is_ready = true;
    status_text =
        "ABI 1 compatible | provider discovery ready | conversation content still disabled";
}

void Win32SecureTransportProvider::shutdown() {
    if (module_handle != NULL) {
        FreeLibrary(module_handle);
        module_handle = NULL;
    }

    is_ready = false;
    capabilities = 0;
    provider_name =
        "Salix Secure Transport Provider";
    status_text =
        "not checked | real content remains blocked";
}

const char* Win32SecureTransportProvider::get_name() const {
    return provider_name.c_str();
}

const char* Win32SecureTransportProvider::get_status_text() const {
    return status_text.c_str();
}

bool Win32SecureTransportProvider::get_is_ready() const {
    return is_ready;
}

unsigned long Win32SecureTransportProvider::get_capabilities() const {
    return capabilities;
}

void Win32SecureTransportProvider::set_unavailable(
    const char* status
) {
    if (module_handle != NULL) {
        FreeLibrary(module_handle);
        module_handle = NULL;
    }

    is_ready = false;
    capabilities = 0;
    provider_name =
        "Salix Secure Transport Provider";
    status_text =
        status == 0 || status[0] == '\0'
            ? "unavailable | real content remains blocked"
            : status;
}

void Win32SecureTransportProvider::set_incompatible(
    const char* status
) {
    if (module_handle != NULL) {
        FreeLibrary(module_handle);
        module_handle = NULL;
    }

    is_ready = false;
    capabilities = 0;
    status_text =
        status == 0 || status[0] == '\0'
            ? "incompatible | real content remains blocked"
            : status;

    // Preserve a provider name if the ABI was readable far enough to
    // retrieve one. This makes cross-toolchain validation observable
    // without treating an incompatible provider as ready.
}
