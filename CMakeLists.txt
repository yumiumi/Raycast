cmake_minimum_required(VERSION 3.28)
project(Raycast)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Raycast main.cpp lighting.cpp imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp rlImGui/rlImGui.cpp)

target_link_libraries(Raycast raylib winmm)
target_include_directories(Raycast PRIVATE SYSTEM raylib-5.5_win64_msvc16/include imgui rlImGui)
target_link_directories(Raycast PRIVATE raylib-5.5_win64_msvc16/lib)
