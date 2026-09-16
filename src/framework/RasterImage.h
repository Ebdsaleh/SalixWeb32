// =================================================================================
// Filename:    framework/RasterImage.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral 32-bit raster image storage.
// =================================================================================
#pragma once

#include <vector>

class RasterImage {
    public:
        RasterImage();

        void clear();
        bool allocate(int width, int height);

        int get_width() const;
        int get_height() const;
        int get_stride() const;

        unsigned char* get_pixels();
        const unsigned char* get_pixels() const;

        bool empty() const;

    private:
        int width;
        int height;
        std::vector<unsigned char> pixels;
};
