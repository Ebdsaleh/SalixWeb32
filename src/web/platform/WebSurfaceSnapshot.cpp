// =================================================================================
// Filename:    web/platform/WebSurfaceSnapshot.cpp
// Author:      Ebdsaleh
// Description: Implements the first backend-neutral WebView surface snapshot.
// =================================================================================

#include "WebSurfaceSnapshot.h"

WebSurfaceSnapshot::WebSurfaceSnapshot() {
    clear();
}

void WebSurfaceSnapshot::clear() {
    title.clear();
    address.clear();
    status.clear();
    content.clear();
}
