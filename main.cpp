#include "raylib.h"
#include "raymath.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include "imgui.h"
#include "rlImGui.h"
#include <vector>
#include "imgui_memory_editor.h"
#include "lighting.hpp"

const int scr_height = 720;
const int scr_width = 1280;
int map_height = 12;
int map_width = 24;
int tile_size = 16;
bool map_editor = false;

enum TileType {
    EMPTY,
    GRAY_WALL,
    RED_WALL,
    GREEN_WALL,
    BLUE_WALL,
};

TileType draw_color;

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

std::vector<TileType> map;

int tile_id(int x, int y) {
    return (y * map_width + x);
}

void parse_map(std::string str) {
    for (int y = 0; y < map_height; y++) {  
        for (int x = 0; x < map_width; x++) {
            int idx = y * map_width + x;
            assert(idx <= map_width * map_height);
            if (str[idx] == '.') {
                map.push_back(EMPTY);
            }
            else if (str[idx] == '#') {
                map.push_back(GRAY_WALL);
            }
            else if (str[idx] == 'B') {
                map.push_back(BLUE_WALL);
            }
            else if (str[idx] == 'R') {
                map.push_back(RED_WALL);
            }
            else if (str[idx] == 'G') {
                map.push_back(GREEN_WALL);
            }
        }
    }
}

ImU32 im_color[5] = {
    IM_COL32(5, 5, 5, 255),
    IM_COL32(100, 100, 100, 255),
    IM_COL32(180, 0, 0, 255),
    IM_COL32(0, 180, 0, 255),
    IM_COL32(0, 0, 180, 255),
};

Color tile_color[5] = {
    BLANK,
    GRAY,
    {201, 27, 27, 255},
    {27, 201, 27, 255},
    {27, 27, 201, 255},
};

linear_color sun = { 2.5f, 2.5f, 2.5f };
Color sky_srgb = { 190, 230, 245, 255 };

Vector2 tile_to_screen(Vector2 pos) {
    float w = (scr_width - map_width * tile_size) * 0.5f;
    float h = (scr_height - map_height * tile_size) * 0.5f;
    pos = Vector2Scale(pos, tile_size);
	return { pos.x + w , pos.y + h };
}

Vector2 screen_to_tile(Vector2 pos) {
    float w = (scr_width - map_width * tile_size) * 0.5f;
    float h = (scr_height - map_height * tile_size) * 0.5f;
    pos = { pos.x - w, pos.y - h };
    return { floor(pos.x / tile_size), floor(pos.y / tile_size) };
}

bool in_bounds(Vector2 v) {
    return (v.y > 0 && v.y < map_height - 1 && v.x > 0 && v.x < map_width - 1);
}

void rebuild_map(int old_width, int old_height) {
    // save old map tiles
    std::vector<TileType> map_copy;
    for (int i = 0; i < map.size(); i++) {
        map_copy.push_back(map[i]);
    }
    // rebuild map with new size:
    // interior = EMPTY, borders = GRAY_WALL
    map.clear();
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            if (in_bounds(Vector2(x, y))) {
                map.push_back(EMPTY);
            }
            else {
                map.push_back(GRAY_WALL);
            }
        }
    }
    // restore old tiles into new map
    // do not overwrite borders or EMPTY cells
    for (int y = 0; y < old_height; y++) {
        for (int x = 0; x < old_width; x++) {
            if (map_copy[y * old_width + x] != EMPTY && map_copy[y * old_width + x] != GRAY_WALL) {
                if (map[y * map_width + x] == EMPTY) {
                    map[y * map_width + x] = map_copy[y * old_width + x];
                }
            }
        }
    }
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
        
        if (IsKeyPressed(KEY_TAB)) {
            map_editor = !map_editor;
        }
        if (map_editor) {
            for (int y = 0; y < map_height; y++) {
                for (int x = 0; x < map_width; x++) {
                    Vector2 pos = { x, y };
                    Vector2 tile_sz = { tile_size, tile_size };
                    TileType tile = map[tile_id(x, y)];
                    DrawRectangleV(tile_to_screen(pos), tile_sz, tile_color[tile]);
                }
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 mpos_tile = screen_to_tile(GetMousePosition());
                if (in_bounds(mpos_tile)) {
                    map[tile_id(int(mpos_tile.x), int(mpos_tile.y))] = draw_color;
                }
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                Vector2 mpos_tile = screen_to_tile(GetMousePosition());
                if (in_bounds(mpos_tile)) {
                    if (map[tile_id((int(mpos_tile.x)),(int(mpos_tile.y)))] != EMPTY) {
                        map[tile_id((int(mpos_tile.x)),(int(mpos_tile.y)))] = EMPTY;
                    }
                }
            }
        }
        else {
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
                    if (map[tile_id(map_x, map_y)] != EMPTY) {
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
                int draw_start = -line_height / 2 + scr_height / 2;
                if (draw_start < 0) {
                    draw_start = 0;
                }
                int draw_end = line_height / 2 + scr_height / 2;
                if (draw_end >= scr_height) {
                    draw_end = scr_height - 1;
                }

                // render walls, floor, sky
                DrawLine(x, draw_start, x, draw_end, get_lighting(tile_color[map[tile_id(map_x, map_y)]], side, sun, sky_srgb));
                DrawLine(x, draw_end, x, scr_height, get_lighting({ 100, 100, 100, 255 }, 1, sun, sky_srgb));
                DrawLine(x, 0, x, draw_start, sky_srgb);
            }

            double delta_time = GetFrameTime();
            float move_speed = delta_time * 5.f;
            float rot_speed = delta_time * 2.f;
            // movement
            if (IsKeyDown(KEY_W)) {
                Vector2 velocity = Vector2Scale(player_dir, move_speed);
                if (map[tile_id(int(player_pos.x + velocity.x), int(player_pos.y))] == EMPTY) {
                    player_pos.x += velocity.x;
                }
                if (map[tile_id(int(player_pos.x), int(player_pos.y + velocity.y))] == EMPTY) {
                    player_pos.y += velocity.y;
                }
            }
            if (IsKeyDown(KEY_S)) {
                Vector2 velocity = Vector2Scale(player_dir, move_speed);
                if (map[tile_id(int(player_pos.x - velocity.x), int(player_pos.y))] == EMPTY) {
                    player_pos.x -= velocity.x;
                }
                if (map[tile_id(int(player_pos.x), int(player_pos.y - velocity.y))] == EMPTY) {
                    player_pos.y -= velocity.y;
                }
            }
            // rotation
            if (IsKeyDown(KEY_A)) {
                player_dir = Vector2Rotate(player_dir, rot_speed);
                camera_plane = Vector2Rotate(camera_plane, rot_speed);
            }
            if (IsKeyDown(KEY_D)) {
                player_dir = Vector2Rotate(player_dir, -rot_speed);
                camera_plane = Vector2Rotate(camera_plane, -rot_speed);
            }
        }

        int map_width_old = map_width;
        int map_height_old = map_height;
        BeginDrawing();
            ClearBackground(BLACK);
            rlImGuiBegin();
                bool open = true;
                ImGui::ShowDemoWindow(&open);
                ImGui::Begin("My_window");
                ImGui::SliderFloat3("sun", (float*)&sun, 0.f, 10.f);

                if (ImGui::InputInt("map_width", &map_width)) {
                    rebuild_map(map_width_old, map_height_old);
                }
                if (ImGui::InputInt("map_height", &map_height)) {
                    rebuild_map(map_width_old, map_height_old);
                }

                int max_color = 3;
                for (int i = 0; i < max_color; i++) {
                    if (i > 0) {
                        ImGui::SameLine();
                    }
                    if (i == 0) {
                        ImGui::PushStyleColor(ImGuiCol_Button, im_color[2]);
                        if (ImGui::Button("red") && map_editor) {
                            draw_color = RED_WALL;
                        }
                    }
                    if (i == 1) {
                        ImGui::PushStyleColor(ImGuiCol_Button, im_color[3]);
                        if (ImGui::Button("green")) {
                            draw_color = GREEN_WALL;
                        }
                    }
                    if (i == 2) {
                        ImGui::PushStyleColor(ImGuiCol_Button, im_color[4]);
                        if (ImGui::Button("blue")) {
                            draw_color = BLUE_WALL;
                        }
                    }
                    ImGui::PopStyleColor(1);
                }

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                const ImVec2 p = ImGui::GetCursorScreenPos();
                float x = p.x + 4.0f;
                float y = p.y + 4.0f;
                static float sz = 12.0f;
                const float spacing = 2.0f;

                // Draw 2d map imgui
                for (int h = 0; h < map_height; h++) {
                    for (int w = 0; w < map_width; w++) {
                        draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), im_color[map[tile_id(w,h)]]);
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
