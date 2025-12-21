#include "lighting.hpp"
#include "raylib.h"
#include "raymath.h"

linear_color srgb_to_lin(Color col) {
    float r = float(col.r) / 255.f;
    float g = float(col.g) / 255.f;
    float b = float(col.b) / 255.f;
    linear_color l_col;
    l_col.r = (r < 0.04045f) ? r * 0.0773993808f : pow(r * 0.9478672986f + 0.0521327014f, 2.4f);
    l_col.g = (g < 0.04045f) ? g * 0.0773993808f : pow(g * 0.9478672986f + 0.0521327014f, 2.4f);
    l_col.b = (b < 0.04045f) ? b * 0.0773993808f : pow(b * 0.9478672986f + 0.0521327014f, 2.4f);
    return l_col;
}

Color lin_to_srgb(linear_color col) {
    Color srgb;
    col.r = col.r / (col.r + 1.f);
    col.g = col.g / (col.g + 1.f);
    col.b = col.b / (col.b + 1.f);
    float r = (col.r < 0.0031308f) ? col.r * 12.92f : 1.055f * pow(col.r, 0.41666f) - 0.055f;
    float g = (col.g < 0.0031308f) ? col.g * 12.92f : 1.055f * pow(col.g, 0.41666f) - 0.055f;
    float b = (col.b < 0.0031308f) ? col.b * 12.92f : 1.055f * pow(col.b, 0.41666f) - 0.055f;
    srgb.r = (r >= 1.f) ? 255 : int(r * 255.f);
    srgb.g = (g >= 1.f) ? 255 : int(g * 255.f);
    srgb.b = (b >= 1.f) ? 255 : int(b * 255.f);
    srgb.a = 255;
    return srgb;
}

Color get_lighting(Color wall, int side, linear_color sun, Color sky_srgb) {
    // albedo * sun * sun_influence + albedo * sky * sky_influence
    float sun_influence = (side == 0) ? 1.f : 0.5f;
    float sky_influence = 0.1f;
    linear_color sky = srgb_to_lin(sky_srgb);
    linear_color wall_albedo = srgb_to_lin(wall);
    linear_color l_lighting;
    l_lighting.r = wall_albedo.r * sun.r * sun_influence + wall_albedo.r * sky.r * sky_influence;
    l_lighting.g = wall_albedo.g * sun.g * sun_influence + wall_albedo.g * sky.g * sky_influence;
    l_lighting.b = wall_albedo.b * sun.b * sun_influence + wall_albedo.b * sky.b * sky_influence;
    Color srgb_lighting = lin_to_srgb(l_lighting);
    return srgb_lighting;
}
