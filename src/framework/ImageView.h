// =================================================================================
// Filename:    framework/ImageView.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral raster image presentation component.
// =================================================================================
#pragma once

#include "Component.h"
#include "RasterImage.h"

class ImageView : public Component {
    public:
        ImageView();

        void set_image(const RasterImage& new_image);
        const RasterImage& get_image() const;
        bool has_image() const;

        virtual void render(ComponentRenderer& renderer) const;

    private:
        RasterImage image;
};
