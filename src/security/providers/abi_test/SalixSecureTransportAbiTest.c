// =================================================================================
// Filename:    security/providers/abi_test/SalixSecureTransportAbiTest.c
// Author:      Ebdsaleh
// Description: Minimal cross-toolchain ABI test provider. No cryptography.
// =================================================================================

#include "security/SalixSecureTransportAbi.h"

unsigned long SALIX_SECURE_TRANSPORT_CALL
salix_secure_transport_get_abi_version(void) {
    return SALIX_SECURE_TRANSPORT_ABI_VERSION;
}

unsigned long SALIX_SECURE_TRANSPORT_CALL
salix_secure_transport_get_capabilities(void) {
    // Deliberately advertise no security capabilities.
    // This provider exists only to validate the DLL/C-ABI boundary on NT 5.2.
    return 0UL;
}

const char* SALIX_SECURE_TRANSPORT_CALL
salix_secure_transport_get_provider_name(void) {
    return "Salix Secure Transport ABI Test Provider";
}
