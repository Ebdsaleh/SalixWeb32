// =================================================================================
// Filename:    framework/Style.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral UI styling primitives.
// =================================================================================
#pragma once

struct Color {
    unsigned char red;
    unsigned char green;
    unsigned char blue;

    Color()
        : red(0), green(0), blue(0) {
    }

    Color(unsigned char new_red, unsigned char new_green, unsigned char new_blue)
        : red(new_red), green(new_green), blue(new_blue) {
    }
};

struct ComponentStyle {
    Color foreground_color;
    Color background_color;
    Color border_color;
    int border_width;

    ComponentStyle()
        : foreground_color(0, 0, 0),
          background_color(240, 240, 240),
          border_color(128, 128, 128),
          border_width(1) {
    }
};
