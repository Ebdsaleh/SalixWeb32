// =================================================================================
// Filename:    web/platform/WebSurfaceSnapshot.cpp
// Author:      Ebdsaleh
// Description: Implements the backend-neutral WebView surface/probe snapshot.
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

    http_status_code = 0;
    http_status_text.clear();
    final_address.clear();
    mime_type.clear();
    response_size = 0;
    response_truncated = false;
    redirect_count = 0;
    script_count = 0;
    form_count = 0;
    link_count = 0;
    response_headers.clear();
    raw_content.clear();
    extracted_content.clear();
}
