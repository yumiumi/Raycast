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

const int tile_size = 1;

int wall_heigh = 2;

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
    "#..BBBBBB..............#"
    "#..B....B.......RRRRRR.#"
    "#..B....B.......R....R.#"
    "#..BBBBBB.......RRRRRR.#"
    "#......................#"
    "#....RRR..GGGGGGGGGG...#"
    "#....R.R..G........G...#"
    "#....RRR..GGGGGGGGGG...#"
    "#......................#"
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

int main() {
    InitWindow(scr_width, scr_height, "Raycast");

    parse_map(map_definition);

    SetTargetFPS(60);
    rlImGuiSetup(true);
    while (!WindowShouldClose()) {
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
                    static float sz = 16.0f;
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
