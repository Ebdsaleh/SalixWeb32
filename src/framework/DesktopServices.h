// =================================================================================
// Filename:    framework/DesktopServices.h
// Author:      Ebdsaleh
// Description: Declares platform desktop services used by application content.
// =================================================================================
#pragma once

class RasterImage;

class DesktopServices {
    public:
        virtual ~DesktopServices() {}

        virtual bool load_image_thumbnail(
            const char* path,
            int maximum_width,
            int maximum_height,
            RasterImage& image
        ) = 0;

        virtual bool open_file(const char* path) = 0;

        virtual bool preview_image(
            const char* path,
            const char* title
        ) = 0;
};
