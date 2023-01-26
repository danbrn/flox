#pragma once

#include <list>
#include <vector>

#include "rect.h"

template <typename A, typename B> struct quad_node_object;
template <typename A, typename C> struct quad_tree_location;
template <typename A, typename D> struct quad_tree_object_location;

template <typename T, typename Object> struct quad_node_object {
    Object  obj;
    rect<T> obj_rect;

    quad_node_object(Object const &o, rect<T> const &r) : obj{o}, obj_rect{r} {}
};

template <typename T, typename Object> struct quad_tree_location {
    std::list<quad_node_object<T, Object>>                   *cont{};
    typename std::list<quad_node_object<T, Object>>::iterator iter{};

    quad_tree_location(
        std::list<quad_node_object<T, Object>>                   *cont,
        typename std::list<quad_node_object<T, Object>>::iterator iter)
        : cont(cont), iter(iter) {}
    quad_tree_location() = default;
};

template <typename T, typename Object> struct quad_tree_object_location {
    using container =
        std::vector<std::optional<quad_tree_object_location<T, Object>>>;
    using container_iter = typename container::iterator;

    Object                                object;
    quad_tree_location<T, container_iter> location;

    explicit quad_tree_object_location(Object const &obj) : object{obj} {}
    explicit quad_tree_object_location(
        Object const &obj, quad_tree_location<T, container_iter> const &loc)
        : object{obj}, location{loc} {}
    quad_tree_object_location() = default;
};

template <typename T, typename Object> class quad_node {
    std::list<quad_node_object<T, Object>>               m_contents{};
    rect<T>                                              m_rect;
    std::array<rect<T>, 4>                               m_child_rects;
    std::array<std::shared_ptr<quad_node<T, Object>>, 4> m_children;
    size_t                                               m_depth;
    size_t                                               m_max_depth;

  public:
    quad_node(rect<T> const &rect, size_t depth, size_t max_depth)
        : m_rect{rect}, m_depth{depth}, m_max_depth(max_depth) {
        vec2d_t<T> child_size = m_rect.size / 2;
        m_child_rects[0]      = {m_rect.position, child_size};
        m_child_rects[1]      = {m_rect.position + vec2d_t<T>{child_size.x, 0},
                                 child_size};
        m_child_rects[2]      = {m_rect.position + vec2d_t<T>{0, child_size.y},
                                 child_size};
        m_child_rects[3]      = {m_rect.position + child_size, child_size};
    }
    quad_node(quad_node const &)                         = delete;
    quad_node(quad_node &&) noexcept                     = default;
    auto operator=(quad_node const &) -> quad_node     & = delete;
    auto operator=(quad_node &&) noexcept -> quad_node & = default;
    ~quad_node()                                         = default;

    auto insert(Object const &obj, rect<T> const &obj_rect)
        -> quad_tree_location<T, Object> {
        for(size_t i = 0; i < 4; ++i) {
            if(m_child_rects[i].contains(obj_rect)) {
                if(m_depth < m_max_depth) {
                    if(!m_children[i]) {
                        m_children[i] = std::make_shared<quad_node<T, Object>>(
                            m_child_rects[i], m_depth + 1, m_max_depth);
                    }
                    return m_children[i]->insert(obj, obj_rect);
                }
            }
        }
        m_contents.emplace_back(obj, obj_rect);
        return {&m_contents, std::prev(m_contents.end())};
    }

    auto size() -> size_t {
        size_t size = m_contents.size();
        for(auto child : m_children) {
            if(child.get() != nullptr) {
                size += child->size();
            }
        }
        return size;
    }

    auto size(rect<T> rect) -> size_t {
        size_t size = std::count_if(
            m_contents.cbegin(), m_contents.cend(),
            [&rect](auto const &obj) { return rect.overlaps(obj.obj_rect); });
        for(size_t i = 0; i < 4; ++i) {
            if(!rect.overlaps(m_child_rects[i])) {
                continue;
            }
            if(m_children[i].get() == nullptr) {
                continue;
            }
            if(rect.contains(m_child_rects[i])) {
                size += m_children[i]->size();
                continue;
            }
            size += m_children[i]->size(rect);
        }
        return size;
    }

    void items(std::vector<Object> &result) {
        for(auto const &qno : m_contents) {
            result.push_back(qno.obj);
        }
        for(auto child : m_children) {
            if(child.get()) {
                child->items(result);
            }
        }
    }

    void items(std::vector<Object> &result, rect<T> const &rect) {
        for(auto qno : m_contents) {
            if(qno.obj_rect.overlaps(rect)) {
                result.push_back(qno.obj);
            }
        }
        for(size_t i = 0; i < 4; ++i) {
            if(m_children[i].get() == nullptr) {
                continue;
            }
            if(!rect.overlaps(m_child_rects[i])) {
                continue;
            }
            if(rect.contains(m_child_rects[i])) {
                m_children[i]->items(result);
                continue;
            }
            m_children[i]->items(result, rect);
        }
    }

    auto rect() const -> rect<T> { return m_rect; }
};

template <typename T, typename Object> class static_quad_tree {
    std::vector<Object>  m_objects;
    size_t               m_max_objects;
    size_t               m_max_depth;
    quad_node<T, Object> m_root;

  public:
    static_quad_tree(rect<T> rect, size_t max_objects, size_t max_depth)
        : m_max_objects{max_objects},
          m_max_depth(max_depth), m_root{rect, 0, max_depth} {
        m_objects.reserve(max_objects);
    }

    void insert(Object const &obj, rect<T> const &obj_rect) {
        m_objects.emplace_back(obj);
        m_root.insert(obj, obj_rect);
    }

    auto size() -> size_t { return m_objects.size(); }

    auto size(rect<T> rect) {
        return rect.contains(m_root.rect()) ? m_root.size() : m_root.size(rect);
    }

    auto items() -> std::vector<Object> const & { return m_objects; }

    auto items(rect<T> rect) -> std::vector<Object> {
        std::vector<Object> result;
        m_root.items(result, rect);
        return result;
    }
};

template <typename T, typename Object> class dynamic_quad_tree {
  public:
    using container =
        std::vector<std::optional<quad_tree_object_location<T, Object>>>;
    using container_iter  = typename container::iterator;
    using container_citer = typename container::const_iterator;

  private:
    container                    m_objects;
    size_t                       m_max_objects;
    size_t                       m_max_depth;
    quad_node<T, container_iter> m_root;
    std::vector<container_iter>  m_removed_objects{};

  public:
    dynamic_quad_tree(rect<T> rect, size_t max_objects, size_t max_depth)
        : m_max_objects{max_objects},
          m_max_depth(max_depth), m_root{rect, 0, max_depth} {
        m_objects.reserve(max_objects);
    }

    [[nodiscard]] auto has_room() const -> bool {
        return m_objects.size() < m_max_objects || m_removed_objects.size() > 0;
    }

    void insert(Object const &obj, rect<T> const &obj_rect) {
        if(!has_room()) {
            throw std::runtime_error{"quad tree full"};
        }
        if(m_objects.size() < m_max_objects) {
            m_objects.emplace_back(obj);
            m_objects.back()->location =
                m_root.insert(std::prev(m_objects.end()), obj_rect);
            return;
        }
        container_iter iter = m_removed_objects.back();
        m_removed_objects.pop_back();
        *iter = quad_tree_object_location{obj, m_root.insert(iter, obj_rect)};
    }

    auto size() -> size_t {
        return m_objects.size() - m_removed_objects.size();
    }

    auto size(rect<T> rect) -> size_t {
        return rect.contains(m_root.rect()) ? m_root.size() : m_root.size(rect);
    }

    [[nodiscard]] auto empty() const -> bool { return m_objects.empty(); }

    auto items() -> std::vector<container_iter> {
        std::vector<container_iter> result;
        for(auto const &o : m_objects) {
            if(o) {
                result.emplace_back(o->location.iter->obj);
            }
        }
        return result;
    }

    auto items(rect<T> rect) -> std::vector<container_iter> {
        if(rect.contains(m_root.rect())) {
            return items();
        }
        std::vector<container_iter> result;
        m_root.items(result, rect);
        return result;
    }

    void remove(container_iter &iter) {
        if(!*iter) {
            return;
        }
        (*iter)->location.cont->erase((*iter)->location.iter);
        m_removed_objects.push_back(iter);
        *iter = std::nullopt;
    }

    void move(container_iter &iter, rect<T> const &rect) {
        if(!*iter) {
            return;
        }
        (*iter)->location.cont->erase((*iter)->location.iter);
        (*iter)->location = m_root.insert(iter, rect);
    }
};
