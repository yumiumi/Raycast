#include "raylib.h"
#include "raymath.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include "imgui.h"
#include "rlImGui.h"
#include "imgui_memory_editor.h"

const int scr_height = 720;
const int scr_width = 1280;
const int map_height = 12;
const int map_width = 24;

enum TileType {
    EMPTY,
    GRAY_WALL,
    RED_WALL,
    BLUE_WALL,
    GREEN_WALL,
};

TileType map[map_height][map_width];

std::string map_definition = {
    "########################"
    "#......................#"
    "#..BBB.................#"
    "#..B.B.................#"
    "#..B.B........RRR......#"
    "#..BBB........R.R......#"
    "#.............RRR......#"
    "#....RRR...............#"
    "#....R.R...............#"
    "#....RRR..GG..GGGGGG...#"
    "#.........GG..GGGGGG...#"
    "########################"
};

void parse_map(std::string str) {
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            int idx = y * map_width + x;
            assert(idx <= map_width * map_height);
            if (str[idx] == '.') {
                map[y][x] = EMPTY;
            }
            else if (str[idx] == '#') {
                map[y][x] = GRAY_WALL;
            }
            else if (str[idx] == 'B') {
                map[y][x] = BLUE_WALL;
            }
            else if (str[idx] == 'R') {
                map[y][x] = RED_WALL;
            }
            else if (str[idx] == 'G') {
                map[y][x] = GREEN_WALL;
            }
        }
    }
}

ImU32 im_color[5] = {
    IM_COL32(5, 5, 5, 255), // black
    IM_COL32(100, 100, 100, 255), // gray
    IM_COL32(180, 0, 0, 255), // red
    IM_COL32(0, 0, 180, 255), // blue
    IM_COL32(0, 180, 0, 255), // green
};

Color w_color[5] = {
    BLANK,
    GRAY,
    {201, 27, 27},
    {27, 27, 201},
    {27, 201, 27},
};

struct linear_color {
    float r;
    float g;
    float b;
};

linear_color sun = { 2.f, 2.f, 2.f };
Color SKY = { 190, 230, 245, 255 }; // color for ClearBackground() too

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

Color get_lighting(Color wall, int side) {
    // albedo * sun * sun_influence + albedo * sky * sky_influence
    float sun_influence = (side == 0) ? 1.f : 0.5f;
    float sky_influence = 0.1f;
    linear_color sky = srgb_to_lin(SKY);
    linear_color wall_albedo = srgb_to_lin(wall);
    linear_color l_lighting;
    l_lighting.r = wall_albedo.r * sun.r * sun_influence + wall_albedo.r * sky.r * sky_influence;
    l_lighting.g = wall_albedo.g * sun.g * sun_influence + wall_albedo.g * sky.g * sky_influence;
    l_lighting.b = wall_albedo.b * sun.b * sun_influence + wall_albedo.b * sky.b * sky_influence;
    Color srgb_lighting = lin_to_srgb(l_lighting);
    return srgb_lighting;
}

int main() {
    InitWindow(scr_width, scr_height, "Raycast");

    parse_map(map_definition);

    Vector2 player_pos = { 20.0f, 3.0f };
    Vector2 player_dir = { -1.f, 0.f };
    Vector2 camera_plane = { 0.f, 0.66f }; // is always perpendicular on the player direction
    Vector2 ray_dir = { 0.f, 0.f };

    SetTargetFPS(60);
    rlImGuiSetup(true);
    while (!WindowShouldClose()) {

        // raycasting loop
        for (int x = 0; x <= scr_width; x++) {
            float camera_x = 2 * x / float(scr_width) - 1; // x-coordinate in camera space (value from -1 to 1)
            player_dir = Vector2Normalize(player_dir);
            camera_plane = Vector2Normalize(camera_plane);

            // half length of camera_plane from given angle
            float fov = std::tan(DEG2RAD * (66.f / 2.f));

            // calculate ray direction
            ray_dir.x = player_dir.x + camera_plane.x * fov * camera_x;
            ray_dir.y = player_dir.y + camera_plane.y * fov * camera_x;
            assert(ray_dir.x != 0.f || ray_dir.y != 0.f);
            ray_dir = Vector2Normalize(ray_dir);

            // length of ray from one x or y-side to next x or y-side
            Vector2 delta_dist;
            // calculate delta distance
            delta_dist.x = std::abs(1.f / ray_dir.x);
            delta_dist.y = std::abs(1.f / ray_dir.y);

            int hit = 0; // was wall hit?
            int side; // which side of wall NS or WE

            // calculate what direction to step in x or y-direction (either +1 or -1)
            int step_x = (ray_dir.x < 0) ? -1 : 1;
            int step_y = (ray_dir.y < 0) ? -1 : 1;

            // current cell we are in
            int map_x = int(player_pos.x);
            int map_y = int(player_pos.y);

            // distance from current position to next x or y-side
            Vector2 side_dist;
            Vector2 fract = Vector2{ player_pos.x - map_x, player_pos.y - map_y };
            side_dist.x = ray_dir.x < 0 ? fract.x : 1.f - fract.x;
            side_dist.y = ray_dir.y < 0 ? fract.y : 1.f - fract.y;
            side_dist = Vector2Multiply(side_dist, delta_dist);

            // perform DDA
            while (hit == 0) {
                if (side_dist.x < side_dist.y) {
                    side_dist.x += delta_dist.x;
                    map_x += step_x;
                    side = 0;
                }
                else {
                    side_dist.y += delta_dist.y;
                    map_y += step_y;
                    side = 1;
                }
                // check if ray has hit a wall
                if (map[map_y][map_x] != EMPTY) {
                    hit = 1;
                }
            }

            float ray_len = (side == 0) ? side_dist.x - delta_dist.x : side_dist.y - delta_dist.y;
            Vector2 ray = Vector2Scale(ray_dir, ray_len);
            // perpendicular distance from wall to camera plane
            float perp_wall_dist = Vector2DotProduct(player_dir, ray);

            // calculate height of line to render
            int line_height = int(scr_height / perp_wall_dist);

            // calculate the lowest and highest pixel to fill in current stripe
            int draw_start = - line_height / 2 + scr_height / 2;
            if (draw_start < 0) {
                draw_start = 0;
            }
            int draw_end = line_height / 2 + scr_height / 2;
            if (draw_end >= scr_height) {
                draw_end = scr_height - 1;
            }

            // render walls
            DrawLine(x, draw_start, x, draw_end, get_lighting(w_color[map[map_y][map_x]], side));
            // render floor
            DrawLine(x, draw_end, x, scr_height, get_lighting({ 100,100,100,255 }, 1));
        }

        double delta_time = GetFrameTime();
        float move_speed = delta_time * 5.f;
        float rot_speed = delta_time * 2.f;
        // movement
        if (IsKeyDown(KEY_W)) {
           Vector2 velocity = Vector2Scale(player_dir, move_speed);
            if(map[int(player_pos.y)][int(player_pos.x + velocity.x)] == EMPTY) {
                player_pos.x += velocity.x;
            }
            if (map[int(player_pos.y + velocity.y)][int(player_pos.x)] == EMPTY) {
                player_pos.y += velocity.y;
            }
        }
        if (IsKeyDown(KEY_S)) {
            Vector2 velocity = Vector2Scale(player_dir, move_speed);
            if (map[int(player_pos.y)][int(player_pos.x - velocity.x)] == EMPTY) {
                player_pos.x -= velocity.x;
            }
            if (map[int(player_pos.y - velocity.y)][int(player_pos.x)] == EMPTY) {
                player_pos.y -= velocity.y;
            }
        }

        if (IsKeyDown(KEY_A)) {
            player_dir = Vector2Rotate(player_dir, rot_speed);
            camera_plane = Vector2Rotate(camera_plane, rot_speed);
        }
        if (IsKeyDown(KEY_D)) {
            player_dir = Vector2Rotate(player_dir, -rot_speed);
            camera_plane = Vector2Rotate(camera_plane, -rot_speed);
        }

        BeginDrawing();
            ClearBackground(SKY);
            rlImGuiBegin();
                bool open = true;
                ImGui::ShowDemoWindow(&open);
                ImGui::Begin("My_window");
                ImGui::SliderFloat3("sun", (float*)&sun, 0.f, 10.f);

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                const ImVec2 p = ImGui::GetCursorScreenPos();
                float x = p.x + 4.0f;
                float y = p.y + 4.0f;
                static float sz = 12.0f;
                const float spacing = 2.0f;

                // Draw 2d map imgui
                for (int h = 0; h < map_height; h++) {
                    for (int w = 0; w < map_width; w++) {
                        draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), im_color[map[h][w]]);
                        x += sz + spacing;
                    }
                    x = p.x + 4.0f;
                    y += sz + spacing;
                }

                ImGui::End();
            rlImGuiEnd();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
