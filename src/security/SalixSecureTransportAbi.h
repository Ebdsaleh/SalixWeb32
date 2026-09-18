// =================================================================================
// Filename:    security/SalixSecureTransportAbi.h
// Author:      Ebdsaleh
// Description: Declares the flat C ABI shared with optional secure transport providers.
// =================================================================================
#pragma once

#define SALIX_SECURE_TRANSPORT_ABI_VERSION 1UL

#define SALIX_SECURE_TRANSPORT_CAP_TLS_1_2              0x00000001UL
#define SALIX_SECURE_TRANSPORT_CAP_TLS_1_3              0x00000002UL
#define SALIX_SECURE_TRANSPORT_CAP_PEER_AUTHENTICATION  0x00000004UL
#define SALIX_SECURE_TRANSPORT_CAP_CERTIFICATE_PINNING  0x00000008UL

#if defined(_WIN32)
#define SALIX_SECURE_TRANSPORT_CALL __cdecl
#else
#define SALIX_SECURE_TRANSPORT_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long (
    SALIX_SECURE_TRANSPORT_CALL*
    SalixSecureTransportGetAbiVersionFunction
)(void);

typedef unsigned long (
    SALIX_SECURE_TRANSPORT_CALL*
    SalixSecureTransportGetCapabilitiesFunction
)(void);

typedef const char* (
    SALIX_SECURE_TRANSPORT_CALL*
    SalixSecureTransportGetProviderNameFunction
)(void);

#ifdef __cplusplus
}
#endif
