#include <bitset>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include <SDL.h>
#include <fmt/core.h>
#include <gfx/gfx.h>

#include "config.h"
#include "constants.h"
#include "types.h"

struct entity_t;

auto key_code(SDL_Keycode k) -> std::optional<key> {
    switch(k) {
    case SDLK_ESCAPE:
    case SDLK_q:
        return key_quit;
    case SDLK_w:
        return key_thrust;
    case SDLK_s:
        return key_reverse;
    case SDLK_a:
        return key_strafe_left;
    case SDLK_d:
        return key_strafe_right;
    case SDLK_LEFT:
        return key_turn_left;
    case SDLK_RIGHT:
        return key_turn_right;
    case SDLK_SPACE:
        return key_fire;
    case SDLK_e:
        return key_center_view;
    case SDLK_PLUS:
        return key_zoom_in;
    case SDLK_MINUS:
        return key_zoom_out;
    case SDLK_0:
        return key_zoom_world;
    case SDLK_1:
        return key_zoom_window;
    case SDLK_UP:
        return key_show_ship;
    case SDLK_DOWN:
        return key_show_boids;
    case SDLK_f:
        return key_show_fps;
    case SDLK_n:
        return key_new_boid;
    case SDLK_p:
        return key_pause;
    case SDLK_h:
        return key_help;
    default:
        return std::nullopt;
    }
}

auto edge_bounce(entity_t &entity) -> bool {
    bool bounced = false;
    if(entity.p_rect.position.x < 0 ||
       entity.p_rect.position.x > world_rect.size.x) {
        entity.velocity.x *= -1;
        bounced           = true;
    }

    if(entity.p_rect.position.y < 0 ||
       entity.p_rect.position.y > world_rect.size.y) {
        entity.velocity.y *= -1;
        bounced           = true;
    }

    if(bounced) {
        entity.p_rect.position.x =
            std::clamp(entity.p_rect.position.x, 0.0, world_rect.size.x);
        entity.p_rect.position.y =
            std::clamp(entity.p_rect.position.y, 0.0, world_rect.size.y);
    }

    return bounced;
}

auto avoid_ship(entity_t &e, entity_t &s) -> vec2d {
    constexpr double avoid_dist    = 200;
    constexpr double avoid_dist_sq = avoid_dist * avoid_dist;

    auto &ep = e.p_rect.position;
    auto &sp = s.p_rect.position;
    if((ep - sp).mag_sq() < avoid_dist_sq) {
        return (sp - ep).norm() * -1 * boid_max_accel;
    }
    return {0, 0};
}

auto input_acceleration(state &st) -> vec2d {
    vec2d acceleration{};
    auto &she = st.ship.entity;
    auto &shp = she.p_rect;
    auto &shh = she.heading;

    acceleration += vec2d::from_angle(shh).with_mag(ship_max_accel) *
                    static_cast<double const>(st.keys_pressed.test(key_thrust));

    acceleration -=
        vec2d::from_angle(shh + M_PI / 2).with_mag(ship_max_accel / 2) *
        static_cast<double const>(st.keys_pressed.test(key_strafe_left));
    acceleration -=
        vec2d::from_angle(shh).with_mag(ship_max_accel / 2) *
        static_cast<double const>(st.keys_pressed.test(key_reverse));
    acceleration +=
        vec2d::from_angle(shh + M_PI / 2).with_mag(ship_max_accel / 2) *
        static_cast<double const>(st.keys_pressed.test(key_strafe_right));
    she.heading -=
        (gfx::modifier_key_pressed(KMOD_SHIFT) ? ship_aim_yaw : ship_max_yaw) *
        static_cast<double>(st.keys_pressed.test(key_turn_left)) *
        st.frame_time;
    she.heading +=
        (gfx::modifier_key_pressed(KMOD_SHIFT) ? ship_aim_yaw : ship_max_yaw) *
        static_cast<double>(st.keys_pressed.test(key_turn_right)) *
        st.frame_time;
    she.velocity += acceleration * st.frame_time;
    she.velocity.limit(ship_max_speed);

    shp.position += she.velocity * st.frame_time;
    if(edge_bounce(she)) {
        she.heading = she.velocity.theta();
    }
    if(st.keys_pressed.test(key_fire)) {
        auto time_from_last_shot =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                st.frame_start_time - st.last_fired)
                .count();
        if(time_from_last_shot >= shot_cooldown_ms) {
            st.last_fired = st.frame_start_time;
            st.shots.entities.emplace_back(
                shp,
                she.velocity + vec2d::from_angle(she.heading) * shot_base_speed,
                she.heading);
        }
    }

    shh = std::fmod(she.heading + 2 * M_PI, 2 * M_PI);

    return acceleration;
}

constexpr auto sq(double x) -> double { return x * x; }

auto outside(vec2d const &p) { return !world_rect.overlaps({p, {1, 1}}); }

auto avoid_edge(entity_t &e) -> vec2d {
    constexpr double delta_angle = M_PI / 2;

    auto const &p          = e.p_rect.position;
    auto const  future_pos = p + e.velocity * 2;
    vec2d       accel{};
    if(outside(future_pos)) {
        e.heading    = std::fmod(e.heading + 2 * M_PI, 2 * M_PI);
        double speed = e.velocity.mag();
        accel        = e.velocity * -boid_max_accel;
        bool first_left{};
        first_left =
            ((future_pos.x < 0 && e.heading <= M_PI) ||
             (future_pos.x >= world_rect.size.x && e.heading > M_PI) ||
             (future_pos.y < 0 && e.heading <= M_PI * 3 / 2) ||
             (future_pos.y >= world_rect.size.y && e.heading <= M_PI / 2));
        vec2d left  = vec2d::from_angle(e.heading - delta_angle);
        vec2d right = vec2d::from_angle(e.heading + delta_angle);
        auto  first = first_left ? left : right;
        if(!outside(p + first * speed * 2)) {
            return first * boid_max_accel;
        }
        auto second = first_left ? right : left;
        if(!outside(p + second * speed * 2)) {
            return second * boid_max_accel;
        }
    }
    return accel;
}

void update_boid_acceleration(state &st, entity_tree::container_iter iter) {
    constexpr double alignment_dist = 250;
    constexpr double cohesion_dist  = 250;

    auto &b  = (*iter)->object; // NOLINT(bugprone-unchecked-optional-access)
    auto &bp = b.p_rect.position;
    b.acceleration = {0, 0};
    b.exploded     = false;
    for(auto const &expl : st.explosions) {
        auto dist_sq = (expl.position - bp).mag_sq();
        dist_sq      = std::max(dist_sq, 1.0);
        auto rad_sq  = sq(explosion_pressure_radius);
        if(dist_sq < rad_sq) {
            vec2d expl_accel = bp - expl.position;
            auto  pressure   = std::min(
                expl.pressure_left, explosion_pressure_per_sec * st.frame_time);
            expl_accel.set_mag(pressure * explosion_pressure_radius /
                               sqrt(dist_sq));
            b.acceleration += expl_accel;
            b.exploded     = true;
        }
    }
    if(b.exploded) {
        return;
    }

    vec2d avoid = avoid_ship(b, st.ship.entity);
    avoid       += avoid_edge(b);
    if(!avoid.is_zero()) {
        b.acceleration = avoid;
        b.acceleration.limit(boid_max_accel);
        return;
    }

    vec2d            alignment_vec{};
    int              alignment_num{};
    vec2d            cohesion_vec{};
    int              cohesion_num{};
    vec2d            separation_vec{};
    constexpr double max_dist = std::max(alignment_dist, cohesion_dist);
    constexpr vec2d  nearby   = {max_dist, max_dist};

    auto nearby_ents = st.boids.entities.items({bp - nearby, nearby * 2});

    for(auto &other : nearby_ents) {
        if(!*other || iter == other) {
            continue;
        }
        auto &o =
            (*other)->object; // NOLINT(bugprone-unchecked-optional-access)
        auto &op      = o.p_rect.position;
        auto  dist_sq = (bp - op).mag_sq();
        if(dist_sq >= sq(cohesion_dist)) {
            continue;
        }
        if(dist_sq < sq(alignment_dist)) {
            alignment_vec += o.velocity;
            ++alignment_num;
        }
        if(dist_sq < sq(cohesion_dist)) {
            cohesion_vec += op;
            ++cohesion_num;
        }
        if(dist_sq < sq(b.separation)) {
            vec2d vec      = bp - op;
            vec            *= std::pow(b.separation, 3) / 2 / dist_sq;
            separation_vec += vec;
        }
    }
    vec2d avg_vel =
        alignment_num == 0 ? b.velocity : alignment_vec / alignment_num;
    vec2d avg_pos = cohesion_num == 0 ? bp : cohesion_vec / cohesion_num;

    avg_vel *= boid_alignment_mult;
    avg_vel.limit(boid_cruise_speed * b.speed_variance);
    b.acceleration = avg_vel - b.velocity;
    b.acceleration += (avg_pos - bp) - b.velocity;
    b.acceleration += separation_vec;

    b.acceleration.limit(boid_max_accel);
}

void decay_explosions(state &st) {
    for(auto &expl : st.explosions) {
        expl.pressure_left -= std::min(
            expl.pressure_left, explosion_pressure_per_sec * st.frame_time);
    }
    st.explosions.erase(
        std::remove_if(begin(st.explosions), end(st.explosions),
                       [](auto &expl) { return expl.pressure_left <= 0.0; }),
        end(st.explosions));
}

void update_boid_position(state &st, entity_tree::container_iter iter) {
    if(!*iter) {
        return;
    }
    auto &b    = (*iter)->object; // NOLINT(bugprone-unchecked-optional-access)
    b.velocity += b.acceleration * st.frame_time;
    if(!b.exploded) {
        b.velocity.limit(boid_max_speed * b.speed_variance);
    }
    b.p_rect.position += b.velocity * st.frame_time;
    edge_bounce(b);
    b.heading = b.velocity.theta();
    st.boids.entities.move(iter, {b.p_rect.position, boid_rect.size});
}

void explode(state &st, vec2d pos) {
    st.explosions.emplace_back(pos, explosion_pressure);
    vec2d radius_rect{explosion_lethal_radius * 2, explosion_lethal_radius * 2};
    for(auto &iter :
        st.boids.entities.items({pos - radius_rect / 2, radius_rect})) {
        if(!*iter) {
            continue;
        }
        auto &e = (*iter)->object; // NOLINT(bugprone-unchecked-optional-access)
        if((e.p_rect.position - pos).mag_sq() < sq(explosion_lethal_radius)) {
            st.boids.entities.remove(iter);
        }
    }
}
void update_shots(state &st) {
    for(auto &shot : st.shots.entities) {
        shot.p_rect.position += shot.velocity * st.frame_time;
        if(!shot.p_rect.overlaps(world_rect)) {
            shot.invalid = true;
        }
    }
    for(auto &shot : st.shots.entities) {
        if(shot.invalid) {
            continue;
        }
        if(st.boids.entities.size(shot.p_rect) > 0) {
            shot.invalid = true;
            explode(st, shot.p_rect.position);
        }
    }
    st.shots.entities.erase(
        std::remove_if(begin(st.shots.entities), end(st.shots.entities),
                       [](auto &shot) { return shot.invalid; }),
        end(st.shots.entities));
}

void update_view(state &st) {
    if(st.view.size.x > world_width) {
        st.view = world_rect;
    }

    auto wp = gfx::world_to_window(st.ship.entity.p_rect.position, st.view,
                                   window_width);
    auto v  = st.ship.entity.velocity;
    auto const &wrs = window_rect.size;
    if((v.x < 0 && wp.x < wrs.x / 3) || (v.x > 0 && wp.x > wrs.x * 2 / 3)) {
        st.view.position.x += st.ship.entity.velocity.x * st.frame_time;
    }
    if((v.y < 0 && wp.y < wrs.y / 3) || (v.y > 0 && wp.y > wrs.y * 2 / 3)) {
        st.view.position.y += st.ship.entity.velocity.y * st.frame_time;
    }

    st.view.clamp(world_rect);
}

void decay_ship_speed(state &st) {
    st.ship.entity.velocity *= 1.0 - st.frame_time;
}

void update(state &st) {
    vec2d acceleration = input_acceleration(st);

    auto boids = st.boids.entities.items();
    for(auto iter : boids) {
        update_boid_acceleration(st, iter);
    }

    decay_explosions(st);

    for(auto iter : boids) {
        update_boid_position(st, iter);
    }

    update_shots(st);

    decay_ship_speed(st);
}

void render(state &st, gfx::renderer &r) {
    r.clear();

    // stars
    r.set_draw_color(gfx::color_light_grey);
    for(auto const &pt : st.stars.items(st.view)) {
        r.draw_point(gfx::world_to_window(pt, st.view, window_width));
    }

    // boids
    for(auto &iter : st.boids.entities.items(st.view)) {
        auto &b = (*iter)->object; // NOLINT(bugprone-unchecked-optional-access)
        r.draw_texture(*st.boids.texture, b.p_rect.position,
                       rad_to_deg(b.heading), st.boids.texture_center, st.view,
                       !st.keys_pressed.test(key_show_boids));
    }

    // shots
    for(auto &s : st.shots.entities) {
        r.draw_texture(*st.shots.texture, s.p_rect.position,
                       rad_to_deg(s.heading), st.shots.texture_center, st.view,
                       !st.keys_pressed.test(key_show_ship));
    }

    // explosions
    for(auto &expl : st.explosions) {
        r.set_draw_color(gfx::color_red);
        double radius = (explosion_pressure - expl.pressure_left) /
                        explosion_pressure * explosion_visual_radius;
        if(st.view.overlaps(rect<double>{expl.position - vec2d{radius, radius},
                                         vec2d{radius * 2, radius * 2}})) {
            r.draw_circle(
                gfx::world_to_window(expl.position, st.view, window_width),
                radius / (st.view.size.x / window_width),
                explosion_circle_segments);
        }
    }

    // ship
    auto &sh = st.ship.entity;
    r.draw_texture(*st.ship.texture, sh.p_rect.position, rad_to_deg(sh.heading),
                   st.ship.texture_center, st.view,
                   !st.keys_pressed.test(key_show_ship));
    if(gfx::modifier_key_pressed(KMOD_SHIFT)) {
        r.set_draw_color(gfx::color_red.with_alpha(gfx::color::mid_value));
        r.draw_line(
            gfx::world_to_window(sh.p_rect.position, st.view, window_width),
            gfx::world_to_window(sh.p_rect.position +
                                     vec2d::from_angle(sh.heading) *
                                         (world_width + world_height),
                                 st.view, window_width));
    }

    // info
    std::string text =
        fmt::format("REM {}/{}", st.boids.entities.size(), number_of_boids);
    if(st.show_fps) {
        text += fmt::format("\nFPS {:.2f}", 1.0 / st.frame_time);
    }
    gfx::color c = gfx::color_green.with_alpha(gfx::color::high_value);
    r.draw_wrapped_text(*st.font, text.c_str(), info_text_location,
                        info_text_width, c);

    // pause message
    if(st.paused && !st.help) {
        if(!st.pause_text) {
            auto       f = gfx::open_font(DATA_PATH "/lcd-font/LCD14.ttf", 160);
            gfx::color c = gfx::color_green.with_alpha(gfx::color::mid_value);
            auto       t = r.text_to_texture<double>(*f, "PAUSED", c);
            st.pause_text     = t;
            st.pause_position = {(window_rect.size.x - t->size().x) / 2,
                                 (window_rect.size.y - t->size().y) / 2};
        }
        r.draw_texture(
            *st.pause_text,
            gfx::window_to_world(st.pause_position, st.view, window_width),
            st.view, false);
    }

    // help page
    if(st.help) {
        if(!st.help_text) {
            auto f =
                gfx::open_font(DATA_PATH "/lcd-font/LCD14.ttf", help_font_size);
            gfx::color c = gfx::color_green.with_alpha(gfx::color::mid_value);
            auto       t = r.wrapped_text_to_texture<double>(*f, help_text, c,
                                                       help_text_width);
            st.help_text = t;
            st.help_position = {(window_rect.size.x - t->size().x) / 2,
                                (window_rect.size.y - t->size().y) / 2};
        }
        r.draw_texture(
            *st.help_text,
            gfx::window_to_world(st.help_position, st.view, window_width),
            st.view, false);
    }

    // cursor
    auto m = static_cast<vec2d>(st.mouse_position);

    const vec2d cursor_offset_1 = {5, 5};
    const vec2d cursor_offset_2 = {-5, 5};
    r.set_draw_color(gfx::color_white);
    r.draw_line(m + cursor_offset_1, m - cursor_offset_1);
    r.draw_line(m + cursor_offset_2, m - cursor_offset_2);

    r.present();
}

void zoom_to(rect<double> &view, vec2d new_size, vec2d new_center = {-1, -1}) {
    vec2d w{window_rect.size};
    if(world_rect.contains({new_center, vec2d{0, 0}})) {
        view = {new_center - new_size / 2, new_size};
    } else {
        view = {gfx::window_to_world(w / 2, view, window_width) - new_size / 2,
                new_size};
    }
}

void handle_mouse_button_event(state &st, SDL_MouseButtonEvent const &e) {
    switch(e.button) {
    case SDL_BUTTON_LEFT:
        explode(st, gfx::window_to_world(static_cast<vec2d>(st.mouse_position),
                                         st.view, window_width));
        break;
    case SDL_BUTTON_RIGHT:
        zoom_to(st.view, window_rect.size,
                gfx::window_to_world(static_cast<vec2d>(st.mouse_position),
                                     st.view, window_width));
        break;
    default:;
    }
}

void handle_keyboard_event(state &st, SDL_KeyboardEvent const &e) {
    auto k = key_code(e.keysym.sym);
    if(k) {
        st.keys_pressed.set(*k, e.state == SDL_PRESSED);
        if(e.state == SDL_RELEASED) {
            switch(*k) {
            case key_show_fps:
                st.show_fps = !st.show_fps;
                break;
            case key_zoom_window:
                zoom_to(st.view, window_rect.size,
                        st.view.position + st.view.size / 2);
                break;
            case key_zoom_world:
                zoom_to(st.view, world_rect.size, st.view.position);
                break;
            case key_pause:
                st.paused = !st.paused;
                break;
            case key_help:
                st.help = !st.help;
                break;
            case key_quit:
                st.quit = true;
                break;
            default:;
            }
        }
    }
}

auto main() -> int {
    gfx::gfx gfx{};
    auto     window =
        gfx::create_window(NAME " " VERSION, window_width, window_height, true);
    auto &renderer = window->get_renderer();

    auto const &win_s   = window_rect.size;
    auto const &world_s = world_rect.size;

    state st{gfx,
             renderer,
             {{(world_s.x - win_s.x) / 2, (world_s.y - win_s.y) / 2}, win_s}};

    gfx::show_cursor(/*visible=*/false);

    st.font = gfx::open_font(DATA_PATH "/lcd-font/LCD14.ttf", info_font_size);
    st.frame_start_time = std::chrono::steady_clock::now();

    SDL_Event e;
    while(!st.quit) {
        while(SDL_PollEvent(&e) != 0) {
            switch(e.type) {
            case SDL_QUIT:
                st.quit = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
                handle_mouse_button_event(st, e.button);
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                handle_keyboard_event(st, e.key);
                break;
            default:
                continue;
            }
        }

        if(st.keys_pressed.test(key_center_view)) {
            st.view.position = st.ship.entity.p_rect.position -
                               vec2d{st.view.size.x / 2, st.view.size.y / 2};
            st.view.clamp(world_rect);
        }

        st.frame_time =
            std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                          st.frame_start_time)
                .count();
        st.frame_start_time = std::chrono::steady_clock::now();

        if(st.keys_pressed.test(key_new_boid)) {
            if(st.boids.entities.has_room()) {
                rect<double> p = {world_rect.size / 2,
                                  static_cast<vec2d>(boid_texture_size)};
                rect<double> r = {world_rect.size / 2, boid_rect.size};
                double       s =
                    gfx.uniform_random_between(boid_max_speed * 0.3,  // NOLINT
                                               boid_max_speed * 0.5); // NOLINT
                double h   = gfx.uniform_random_between(0, M_PI * 2);
                double sep = gfx.uniform_random_between(
                    boid_average_separation * 0.8,                     // NOLINT
                    boid_average_separation * 1.3);                    // NOLINT
                double s_var = gfx.uniform_random_between(0.75, 1.25); // NOLINT
                vec2d  v     = vec2d::from_angle(h) * s;
                st.boids.entities.insert({p, v, h, sep, s_var}, r);
            }
        }

        if(st.keys_pressed.test(key_zoom_in)) {
            zoom_to(st.view,
                    st.view.size / (1 + zoom_per_second * st.frame_time));
        }
        if(st.keys_pressed.test(key_zoom_out)) {
            zoom_to(st.view,
                    st.view.size * (1 + zoom_per_second * st.frame_time));
        }
        st.mouse_buttons =
            gfx::get_mouse_state(st.mouse_position.x, st.mouse_position.y);

        if(!st.paused) {
            update(st);
        }

        update_view(st);

        render(st, renderer);
    }
    return 0;
}
