#include "raylib.h"
#include "raymath.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include "imgui.h"
#include "rlImGui.h"
#include "imgui_memory_editor.h"

const int scr_height = 600;
const int scr_width = 1024;
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
    RED,
    BLUE,
    GREEN,
};

Color wall_col(TileType wall, int side) {
    return (side == 0) ? w_color[wall] : ColorBrightness(w_color[wall], -0.25f);
}

int main() {
    InitWindow(scr_width, scr_height, "Raycast");

    parse_map(map_definition);

    Vector2 player_pos = { 20.0f, 3.0f };
    Vector2 player_dir = { -1.f, 0.f };
    Vector2 camera_plane = { 0.f, 0.66f }; // is always perpendicular on the player direction
    Vector2 ray_dir = { 0.f, 0.f };

    double cur_time = 0.f;
    double old_time = 0.f;

    SetTargetFPS(60);
    rlImGuiSetup(true);
    while (!WindowShouldClose()) {

        // raycasting loop
        for (int x = 0; x < scr_width; x++) {
            // calculate ray position and direction
            float camera_x = 2 * x / float(scr_width) - 1; // x-coordinate in camera space (value from -1 to 1)
            player_dir = Vector2Normalize(player_dir);
            camera_plane = Vector2Normalize(camera_plane);
            // calculate half length of camera_plane from given angle
            float fov = std::tan(DEG2RAD * (90.0f / 2.f));
            ray_dir.x = player_dir.x + camera_plane.x * fov * camera_x;
            ray_dir.y = player_dir.y + camera_plane.y * fov * camera_x;

            assert(ray_dir.x != 0.f || ray_dir.y != 0.f);
            ray_dir = Vector2Normalize(ray_dir);

            // length of ray from one x or y-side to next x or y-side
            Vector2 delta_dist;
            // calculate delta distance
            delta_dist.x = std::abs(1.f / ray_dir.x);
            delta_dist.y = std::abs(1.f / ray_dir.y);

            // length of ray from current position to next x or y-side
            Vector2 side_dist;
            // what direction to step? in x or y-direction (either +1 or -1)
            int step_x;
            int step_y;
            // current cell we are in
            int map_x = int(player_pos.x);
            int map_y = int(player_pos.y);

            int hit = 0; // was wall hit?
            int side; // which side of wall NS or WE

            // calculate step and initial side distance
            if (ray_dir.x < 0) {
                step_x = -1;
                side_dist.x = (player_pos.x - map_x) * delta_dist.x;
            }
            else {
                step_x = 1;
                side_dist.x = (map_x + 1.f - player_pos.x) * delta_dist.x;
            }
            if (ray_dir.y < 0) {
                step_y = -1;
                side_dist.y = (player_pos.y - map_y) * delta_dist.y;
            }
            else {
                step_y = 1;
                side_dist.y = (map_y + 1.f - player_pos.y) * delta_dist.y;
            }

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

            // perpendicular distance from wall to mera plane
            float ray_len;
            // calculate distance projected on camera direcion
            if (side == 0) {
                ray_len = side_dist.x - delta_dist.x;
            }
            else {
                ray_len = side_dist.y - delta_dist.y;
            }
            Vector2 ray = Vector2Scale(ray_dir, ray_len);

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
            if (map[map_y][map_x] != EMPTY) {
                DrawLine(x, draw_start, x, draw_end, wall_col(map[map_y][map_x], side));
            }
        }

        old_time = cur_time;
        cur_time = GetTime();
        //float delta_time = GetFrameTime();
        double frame_time = (cur_time - old_time);

        double move_speed = frame_time * 5.f;
        double rot_speed = frame_time * 2.f;
        // movement
        if (IsKeyDown(KEY_W)) {
            if (map[int(player_pos.y)][int(player_pos.x + player_dir.x * move_speed)] == EMPTY) {
                player_pos.x += player_dir.x * move_speed;
            }
            if (map[int(player_pos.y + player_dir.y * move_speed)][int(player_pos.x)] == EMPTY) {
                player_pos.y += player_dir.y * move_speed;
            }
        }
        if (IsKeyDown(KEY_S)) {
            if (map[int(player_pos.y)][int(player_pos.x - player_dir.x * move_speed)] == EMPTY) {
                player_pos.x -= player_dir.x * move_speed;
            }
            if (map[int(player_pos.y - player_dir.y * move_speed)][int(player_pos.x)] == EMPTY) {
                player_pos.y -= player_dir.y * move_speed;
            }
        }

        if (IsKeyDown(KEY_A)) {
            double old_dir_x = player_dir.x;
            player_dir.x = player_dir.x * cos(rot_speed) - player_dir.y * sin(rot_speed);
            player_dir.y = old_dir_x * sin(rot_speed) + player_dir.y * cos(rot_speed);
            double old_plane_x = camera_plane.x;
            camera_plane.x = camera_plane.x * cos(rot_speed) - camera_plane.y * sin(rot_speed);
            camera_plane.y = old_plane_x * sin(rot_speed) + camera_plane.y * cos(rot_speed);
        }
        if (IsKeyDown(KEY_D)) {
            double old_dir_x = player_dir.x;
            player_dir.x = player_dir.x * cos(-rot_speed) - player_dir.y * sin(-rot_speed);
            player_dir.y = old_dir_x * sin(-rot_speed) + player_dir.y * cos(-rot_speed);
            double old_plane_x = camera_plane.x;
            camera_plane.x = camera_plane.x * cos(-rot_speed) - camera_plane.y * sin(-rot_speed);
            camera_plane.y = old_plane_x * sin(-rot_speed) + camera_plane.y * cos(-rot_speed);
        }


        BeginDrawing();
            ClearBackground(BLACK);
            rlImGuiBegin();
                bool open = true;
                ImGui::ShowDemoWindow(&open);
                ImGui::Begin("My_window");

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
