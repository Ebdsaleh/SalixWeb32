// =================================================================================
// Filename:    framework/ImageView.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral raster image presentation component.
// =================================================================================

#include "ImageView.h"
#include "rendering/ComponentRenderer.h"

ImageView::ImageView() {
    get_style().background_color = Color(245, 245, 245);
    get_style().border_color = Color(165, 165, 165);
    get_style().border_width = 1;
}

void ImageView::set_image(const RasterImage& new_image) {
    image = new_image;
}

const RasterImage& ImageView::get_image() const {
    return image;
}

bool ImageView::has_image() const {
    return !image.empty();
}

void ImageView::render(ComponentRenderer& renderer) const {
    if (!get_is_visible()) {
        return;
    }

    renderer.render_image_view(*this);
}
