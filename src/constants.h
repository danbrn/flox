#pragma once

#include "types.h"

constexpr auto tsize_to_rect(vec2d_t<int> texture_size) -> rect<double> {
    auto max = static_cast<double>(std::max(texture_size.x, texture_size.y));
    return {{0, 0}, {max, max}};
}

constexpr int      window_width  = 1280;
constexpr int      window_height = 960;
rect<double> const window_rect{{0.0, 0.0}, {window_width, window_height}};

constexpr int      world_width  = 8 * window_width;
constexpr int      world_height = 8 * window_height;
rect<double> const world_rect{{0.0, 0.0}, {world_width, world_height}};

constexpr size_t number_of_boids = 5000;
constexpr size_t number_of_stars = 1000;

constexpr double zoom_per_second = 1;

constexpr size_t quad_tree_max_depth = 7;

constexpr double       ship_max_speed      = 1500.0;
constexpr double       ship_max_accel      = 600.0;
constexpr double       ship_max_yaw        = M_PI;
constexpr double       ship_aim_yaw        = 0.2 * ship_max_yaw;
constexpr vec2d        ship_texture_center = {20, 10};
constexpr vec2d_t<int> ship_texture_size   = {40, 20};
constexpr rect<double> ship_rect{tsize_to_rect(ship_texture_size)};

constexpr double       boid_max_speed          = 800.0;
constexpr double       boid_cruise_speed       = 350.0;
constexpr double       boid_max_accel          = 500.0;
constexpr double       boid_average_separation = 55.0;
constexpr double       boid_alignment_mult     = 2.75;
constexpr vec2d        boid_texture_center     = {20, 10};
constexpr vec2d_t<int> boid_texture_size       = {40, 20};
constexpr rect<double> boid_rect{tsize_to_rect(boid_texture_size)};

constexpr double       shot_base_speed  = 2400;
constexpr size_t       shot_cooldown_ms = 100;
constexpr vec2d_t<int> shot_texture_size{10, 5};
constexpr rect<double> shot_rect{tsize_to_rect(shot_texture_size)};

constexpr double explosion_lethal_radius    = 60;
constexpr double explosion_pressure_radius  = 300;
constexpr double explosion_visual_radius    = 150;
constexpr size_t explosion_circle_segments  = 20;
constexpr double explosion_pressure         = 8000;
constexpr double explosion_pressure_per_sec = 60000;

constexpr vec2d  info_text_location{10, 10};
constexpr size_t info_text_width =
    static_cast<size_t>(window_width - 2 * info_text_location.x);
constexpr size_t info_font_size = 18;

constexpr size_t      help_font_size  = 24;
constexpr size_t      help_text_width = 840;
constexpr char const *help_text =
    "W - THRUST                      UP - SHOW SHIP\n"
    "S - REVERSE THRUST            DOWN - SHOW BOIDS\n"
    "A - STRAFE LEFT               LEFT - YAW LEFT\n"
    "D - STRAFE RIGHT             RIGHT - YAW RIGHT\n"
    "Q - QUIT                     SHIFT - AIM\n"
    "H - TOGGLE HELP              SPACE - FIRE\n"
    "P - TOGGLE PAUSE                 N - NEW BOID\n"
    "+ - ZOOM IN                      1 - ZOOM WINDOW\n"
    "- - ZOOM OUT                     0 - ZOOM WORLD\n"
    "E - CENTER SHIP                  F - TOGGLE FPS\n";
