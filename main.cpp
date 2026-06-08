#include <cmath>
#include <print>
#include <unordered_set>
#include <utility>
#include <string>
#include <format>

#include "raylib.h"
#include "raymath.h"

constexpr float sign(float value) {
    if (value < 0) {
        return -1.0f;
    }
    return 1.0f;
}

constexpr float fractional(float value) {
    return sign(value) * (std::abs(value) - std::floor(std::abs(value)));
}

struct Vector2i {
    int x = 0;
    int y = 0;

    constexpr Vector2i &operator+=(Vector2i other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    friend constexpr Vector2i operator+(Vector2i a, Vector2i b) {
        a += b;
        return a;
    }

    friend constexpr bool operator==(Vector2i a, Vector2i b) = default;
};

template <>
struct std::hash<Vector2i> {
    constexpr size_t operator()(Vector2i vec) const {
        return vec.x ^ (vec.y << 1);
    }
};

struct World {
    std::unordered_set<Vector2i> live_cells;

    bool is_live_cell(Vector2i position) {
        return live_cells.contains(position);
    }

    void add_live_cell(Vector2i position) {
        live_cells.insert(position);
    }

    void remove_live_cell(Vector2i position) {
        live_cells.erase(position);
    }

    void clear() {
        live_cells.clear();
    }

    int count_live_cells_around_cell(Vector2i position) {
        int count = 0;
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                if (x == y && x == 0) {
                    continue;
                }

                if (is_live_cell(position + Vector2i{x, y})) {
                    count += 1;
                }
            }
        }
        return count;
    }
};

constexpr auto CELL_SIZE = 40;

int main() {
    InitWindow(1600, 900, "Game Of Life");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    int monitor = GetCurrentMonitor();
    SetTargetFPS(GetMonitorRefreshRate(monitor));

    auto camera = Camera2D{
        .offset = Vector2Zeros,
        .target = Vector2Zeros,
        .rotation = 0.0f,
        .zoom = 1.0f,
    };
    auto world1 = World{.live_cells = {
                            {0, 0},
                            {1, 0},
                            {1, -1},
                            {1, -2},
                            {2, -2},
                            {2, -3},
                            {3, -3},
                        }};
    auto world2 = World{};
    auto current_world = &world1;
    auto next_world = &world2;
    float tick_timer = 0.0f;
    float tick_interval_secs = 0.1f;
    bool simulate = false;
    int generation = 0;

    bool mouse_left_down = false;
    bool mouse_left_down_started_on_live = false;
    bool grid = true;
    bool show_info = true;
    bool limit_fps = true;

    std::string format_buffer;
    format_buffer.reserve(256);
    
    while (!WindowShouldClose()) {
        // Input/Update
        auto new_monitor = GetCurrentMonitor();
        auto dt = GetFrameTime();
        auto width = GetScreenWidth();
        auto height = GetScreenHeight();
        auto mouse_position = GetMousePosition();
        auto mouse_world_position = GetScreenToWorld2D(mouse_position, camera);
        auto mouse_cell =
            Vector2i{static_cast<int>(std::floor(mouse_world_position.x / CELL_SIZE)),
                     static_cast<int>(std::floor(mouse_world_position.y / CELL_SIZE))};
        bool tick = false;
        auto wheel = GetMouseWheelMove();

        auto update_timer = [dt](float &timer, float limit) -> bool {
            if (limit <= std::numeric_limits<float>::epsilon()) {
                return true;
            }

            bool result = false;
            timer += dt;
            while (timer >= limit) {
                timer -= limit;
                result = true;
            }
            return result;
        };

        if (monitor != new_monitor) {
            if (limit_fps) {
                SetTargetFPS(GetMonitorRefreshRate(monitor));
            }
        }

        if (wheel != 0.0f) {
            camera.offset = mouse_position;
            camera.target = mouse_world_position;

            constexpr auto SCROLL_FACTOR = 0.13f;
            constexpr auto MAX_ZOOM_OUT = 0.01f;
            auto scale = wheel * SCROLL_FACTOR;
            camera.zoom =
                std::max(std::exp(std::log(camera.zoom) + scale), MAX_ZOOM_OUT);
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            auto delta = GetMouseDelta();
            delta *= -1.0f / camera.zoom;
            camera.target += delta;
        }

        if (IsKeyPressed(KEY_RIGHT)) {
            tick = true;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            simulate = !simulate;
            tick_timer = 0.0f;
        }

        if (IsKeyPressed(KEY_UP)) {
            tick_interval_secs += 0.05f;
        }

        if (IsKeyPressed(KEY_DOWN)) {
            tick_interval_secs -= 0.05f;
            tick_interval_secs = std::max(tick_interval_secs, 0.0f);
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            mouse_left_down = true;
            mouse_left_down_started_on_live = current_world->is_live_cell(mouse_cell); 

            if (!current_world->is_live_cell(mouse_cell)) {
                current_world->add_live_cell(mouse_cell);
            } else {
                current_world->remove_live_cell(mouse_cell);
            }
        }

        if (IsKeyPressed(KEY_G)) {
            grid = !grid;
        }

        if (IsKeyPressed(KEY_H)) {
            show_info = !show_info;
        }
        
        if (IsKeyPressed(KEY_F)) {
            limit_fps = !limit_fps;
            if (limit_fps) {
                SetTargetFPS(GetMonitorRefreshRate(monitor));
            } else {
                SetTargetFPS(0);
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            mouse_left_down = false;
        }

        if (mouse_left_down) {
            if (mouse_left_down_started_on_live) {
                current_world->remove_live_cell(mouse_cell);
            } else {
                current_world->add_live_cell(mouse_cell);
            }
        }

        if (simulate) {
            tick_timer += dt;
            tick = update_timer(tick_timer, tick_interval_secs);
        }

        if (tick) {
            generation += 1;
            next_world->clear();
            for (auto live_cell : current_world->live_cells) {
                {
                    auto live_cells_around =
                        current_world->count_live_cells_around_cell(live_cell);
                    if (live_cells_around == 2 || live_cells_around == 3) {
                        next_world->add_live_cell(live_cell);
                    }
                }

                for (int y = -1; y <= 1; ++y) {
                    for (int x = -1; x <= 1; ++x) {
                        if (x == y && x == 0) {
                            continue;
                        }

                        auto cell = live_cell + Vector2i{x, y};
                        auto live_cells_around =
                            current_world->count_live_cells_around_cell(cell);
                        if (live_cells_around == 3) {
                            next_world->add_live_cell(cell);
                        }
                    }
                }
            }
            std::swap(current_world, next_world);
        }

        // Render
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);
        constexpr auto CELL_COLOR = PINK;
        for (auto cell : current_world->live_cells) {
            DrawRectangle(cell.x * CELL_SIZE, cell.y * CELL_SIZE, CELL_SIZE,
                          CELL_SIZE, CELL_COLOR);
        }
        EndMode2D();

        constexpr auto GRID_SPACING = 40;
        constexpr auto GRID_COLOR = GRAY;

        auto spacing = camera.zoom * GRID_SPACING;

        bool draw_grid = grid && spacing > 5;
        if (draw_grid) {
            {
                auto camera_offset_from_origin =
                    camera.target.x * camera.zoom - camera.offset.x;
                auto offset =
                    fractional(camera_offset_from_origin / spacing) * spacing;
                for (int i = 0; i < static_cast<int>((width / spacing)) + 2; ++i) {
                    auto line_start = Vector2{-offset + i * spacing, 0};
                    auto line_end =
                        Vector2{-offset + i * spacing, static_cast<float>(height)};
                    DrawLineV(line_start, line_end, GRID_COLOR);
                }
            }

            {
                auto camera_offset_from_origin =
                    camera.target.y * camera.zoom - camera.offset.y;
                auto offset =
                    fractional(camera_offset_from_origin / spacing) * spacing;
                for (int i = 0; i < static_cast<int>((height / spacing)) + 2; ++i) {
                    auto line_start = Vector2{0, -offset + i * spacing};
                    auto line_end = Vector2{static_cast<float>(width),
                                            -offset + i * spacing};
                    DrawLineV(line_start, line_end, GRID_COLOR);
                }
            }
        }

        if (show_info) {
            format_buffer.clear();
            std::format_to(
                std::back_inserter(format_buffer),
                "Tick interval: {:.2f} seconds\nGeneration: {}\nLive count: {}\nFPS: {:.0f}",
                tick_interval_secs, generation, current_world->live_cells.size(), 1.0f / dt);

            constexpr auto FONT_SIZE = 22;
            constexpr auto TEXT_MARGIN = 5;
            auto text_size = MeasureTextEx(GetFontDefault(), format_buffer.c_str(),
                                        FONT_SIZE, 2.0f);
            DrawRectangleV(Vector2{0.0f, 0.0f}, text_size + Vector2{2.0f * TEXT_MARGIN, 2.0f * TEXT_MARGIN},
                        ColorFromNormalized(Vector4{0.0f, 0.0f, 0.0f, 0.2f}));
            DrawText(format_buffer.c_str(), TEXT_MARGIN, TEXT_MARGIN, FONT_SIZE, BLACK);
        }
        
        EndDrawing();
    }

    CloseWindow();

    return 0;
}