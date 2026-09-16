// =================================================================================
// Filename:    framework/RasterImage.cpp
// Author:      Ebdsaleh
// Description: Implements backend-neutral 32-bit raster image storage.
// =================================================================================

#include "RasterImage.h"

RasterImage::RasterImage()
    : width(0),
      height(0) {
}

void RasterImage::clear() {
    width = 0;
    height = 0;
    pixels.clear();
}

bool RasterImage::allocate(int new_width, int new_height) {
    clear();

    if (new_width <= 0 || new_height <= 0) {
        return false;
    }

    unsigned long byte_count =
        (unsigned long)new_width *
        (unsigned long)new_height *
        4UL;

    if (byte_count == 0UL) {
        return false;
    }

    pixels.resize((std::vector<unsigned char>::size_type)byte_count);

    width = new_width;
    height = new_height;
    return true;
}

int RasterImage::get_width() const {
    return width;
}

int RasterImage::get_height() const {
    return height;
}

int RasterImage::get_stride() const {
    return width * 4;
}

unsigned char* RasterImage::get_pixels() {
    return pixels.empty() ? 0 : &pixels[0];
}

const unsigned char* RasterImage::get_pixels() const {
    return pixels.empty() ? 0 : &pixels[0];
}

bool RasterImage::empty() const {
    return width <= 0 || height <= 0 || pixels.empty();
}
