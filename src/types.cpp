#include "types.h"
#include "constants.h"

auto create_ship_texture(gfx::renderer &r) -> std::shared_ptr<gfx::texture> {
    auto texture =
        gfx::create_texture(r, ship_texture_size.x, ship_texture_size.y);
    r.set_target(*texture);
    r.clear();
    r.set_draw_color(gfx::color::max_value, gfx::color::max_value,
                     gfx::color::max_value, SDL_ALPHA_OPAQUE);
    // NOLINTBEGIN
    std::vector<SDL_Point> points{{0, 9},  {39, 9}, {0, 0}, {0, 19},
                                  {39, 9}, {1, 0},  {1, 5}, {39, 9},
                                  {1, 14}, {1, 19}, {39, 9}};
    // NOLINTEND
    r.draw_lines(points);
    r.reset_target();
    return texture;
}

auto create_boid_texture(gfx::renderer &r) -> std::shared_ptr<gfx::texture> {
    auto texture =
        gfx::create_texture(r, boid_texture_size.x, boid_texture_size.y);
    r.set_target(*texture);
    r.clear();
    r.set_draw_color(gfx::color::max_value, gfx::color::max_value / 2,
                     gfx::color::max_value / 2, SDL_ALPHA_OPAQUE);
    std::vector<SDL_Point> points{
        {0, 9}, {39, 9}, {0, 0}, {0, 19}, {39, 9}}; // NOLINT
    r.draw_lines(points);
    r.reset_target();
    return texture;
}

auto create_shot_texture(gfx::renderer &r) -> std::shared_ptr<gfx::texture> {
    auto texture =
        gfx::create_texture(r, shot_texture_size.x, shot_texture_size.y);
    r.set_target(*texture);
    r.clear();
    r.set_draw_color(gfx::color::max_value, 0, 0, SDL_ALPHA_OPAQUE);
    std::vector<SDL_Point> points{
        {0, 2}, {9, 2}, {0, 0}, {0, 4}, {9, 2}}; // NOLINT
    r.draw_lines(points);
    r.reset_target();
    return texture;
}

auto create_ship(
    gfx::renderer &r,
    rect<double>   p = {{world_rect.size / 2},
                        {static_cast<vec2d const>(ship_texture_size)}},
    vec2d v = {0.0, 0.0}, double h = 0.0) -> ship_t {
    return ship_t{{p, v, h}, create_ship_texture(r), ship_texture_center};
}

auto create_boids(gfx::gfx &gfx, gfx::renderer &r) -> boids_t {
    boids_t boids{{world_rect, number_of_boids, quad_tree_max_depth},
                  create_boid_texture(r),
                  boid_texture_center};
    for(size_t i = 0; i < number_of_boids; ++i) {
        rect<double> position = {{gfx.uniform_random_between(0, world_width),
                                  gfx.uniform_random_between(0, world_height)},
                                 static_cast<vec2d>(boid_texture_size)};
        rect<double> rect     = {position.position, boid_rect.size};
        double       speed =
            gfx.uniform_random_between(boid_max_speed * 0.3,  // NOLINT
                                       boid_max_speed * 0.5); // NOLINT
        double heading = gfx.uniform_random_between(0, M_PI * 2);
        double separation =
            gfx.uniform_random_between(boid_average_separation * 0.8,  // NOLINT
                                       boid_average_separation * 1.3); // NOLINT
        double speed_var = gfx.uniform_random_between(0.75, 1.25);     // NOLINT
        vec2d  velocity  = {speed * cos(heading), speed * sin(heading)};
        boids.entities.insert(
            {position, velocity, heading, separation, speed_var}, rect);
    }
    return boids;
}

auto create_stars(gfx::gfx &gfx) -> star_tree {
    star_tree star_tree{world_rect, number_of_stars, quad_tree_max_depth};
    for(size_t i = 0; i < number_of_stars; ++i) {
        vec2d        p{gfx.uniform_random_between(0, world_rect.size.x),
                gfx.uniform_random_between(0, world_rect.size.y)};
        rect<double> r{p, {1, 1}};
        star_tree.insert(p, r);
    }
    return star_tree;
}

auto create_shots(gfx::renderer &r) -> shots_t {
    return {create_shot_texture(r), static_cast<vec2d>(shot_texture_size) / 2};
}

state::state(gfx::gfx &gfx, gfx::renderer &r, rect<double> view = window_rect)
    : ship{create_ship(r)}, boids{create_boids(gfx, r)}, shots{create_shots(r)},
      stars{create_stars(gfx)}, view(view) {}
