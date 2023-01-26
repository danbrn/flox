#pragma once

#include <bitset>
#include <gfx/font.h>
#include <gfx/gfx.h>

#include "quad_tree.h"
#include "rect.h"
#include "vec2d.h"

using vec2d = vec2d_t<double>;

using time_point = std::chrono::time_point<std::chrono::steady_clock>;

struct entity_t {
    rect<double> p_rect{};
    vec2d        velocity{};
    vec2d        acceleration{};
    double       heading{};
    double       separation{};
    double       speed_variance{};
    bool         exploded{false};

    entity_t()                                     = default;
    entity_t(entity_t const &)                     = default;
    entity_t(entity_t &&)                          = default;
    auto operator=(entity_t const &) -> entity_t & = default;
    auto operator=(entity_t &&) -> entity_t      & = default;
    entity_t(rect<double> p, vec2d v = {0.0, 0.0}, // NOLINT
             double h = 0.0, double s = 0.0, double sv = 1.0)
        : p_rect{p}, velocity{v}, heading{h}, separation{s}, speed_variance{
                                                                 sv} {}
    ~entity_t() = default;
};

struct ship_t {
    entity_t                      entity;
    std::shared_ptr<gfx::texture> texture;
    vec2d                         texture_center;
};

using entity_tree = dynamic_quad_tree<double, entity_t>;

struct boids_t {
    entity_tree                   entities;
    std::shared_ptr<gfx::texture> texture;
    vec2d                         texture_center;
};

struct shot_t {
    rect<double> p_rect{};
    vec2d        velocity{};
    double       heading{};
    bool         invalid{false};

    shot_t(rect<double> const &r, vec2d const &v, double h)
        : p_rect{r}, velocity{v}, heading{h} {}
};

struct shots_t {
    std::shared_ptr<gfx::texture> texture;
    vec2d                         texture_center;

    std::vector<shot_t> entities;
};

struct explosion_t {
    vec2d  position;
    double pressure_left;

    explosion_t(vec2d const &p, double pl) : position(p), pressure_left(pl) {}
};

enum key {
    key_quit = 0,
    key_thrust,
    key_reverse,
    key_strafe_left,
    key_strafe_right,
    key_up,
    key_down,
    key_turn_left,
    key_turn_right,
    key_fire,
    key_center_view,
    key_zoom_in,
    key_zoom_out,
    key_zoom_world,
    key_zoom_window,
    key_show_ship,
    key_show_boids,
    key_show_fps,
    key_new_boid,
    key_pause,
    key_help,
    key_count
};

using star_tree = static_quad_tree<double, vec2d>;

struct state {
    vec2d                         mouse_position{};
    uint32_t                      mouse_buttons{};
    ship_t                        ship;
    boids_t                       boids;
    shots_t                       shots;
    std::vector<explosion_t>      explosions;
    star_tree                     stars;
    time_point                    last_fired{};
    std::bitset<key_count>        keys_pressed{};
    time_point                    frame_start_time;
    double                        frame_time{};
    rect<double>                  view;
    std::shared_ptr<gfx::font>    font;
    std::shared_ptr<gfx::texture> pause_text{};
    vec2d                         pause_position{};
    std::shared_ptr<gfx::texture> help_text{};
    vec2d                         help_position{};
    bool                          show_fps{false};
    bool                          quit{false};
    bool                          paused{false};
    bool                          help{false};

    explicit state(gfx::gfx &gfx, gfx::renderer &r, rect<double> view);
};
