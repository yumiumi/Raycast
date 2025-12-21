#pragma once
#include "raylib.h"

struct linear_color {
    float r;
    float g;
    float b;
};

linear_color srgb_to_lin(Color col);
Color lin_to_srgb(linear_color col);
Color get_lighting(Color wall, int side, linear_color sun, Color SKY);
