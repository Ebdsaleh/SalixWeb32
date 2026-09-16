// =================================================================================
// Filename:    engine/platform/win32/Win32DesktopServices.h
// Author:      Ebdsaleh
// Description: Declares Win32 file, preview, and thumbnail desktop services.
// =================================================================================
#pragma once

#include <windows.h>
#include <vector>

#include "framework/DesktopServices.h"

class Win32DesktopServices : public DesktopServices {
    public:
        Win32DesktopServices(HINSTANCE instance_handle);

        virtual bool load_image_thumbnail(
            const char* path,
            int maximum_width,
            int maximum_height,
            RasterImage& image
        );

        virtual bool open_file(const char* path);

        virtual bool preview_image(
            const char* path,
            const char* title
        );

    private:
        HINSTANCE instance_handle;
};
