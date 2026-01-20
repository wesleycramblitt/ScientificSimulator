#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <tuple>
#include <utility>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <functional>

struct Entity {
    using id_type = std::uint32_t;
    using gen_type = std::uint32_t;

    id_type id{std::numeric_limits<id_type>::max()};
    gen_type gen{0};

    friend bool operator==(const Entity& a, const Entity& b) noexcept {
        return a.id == b.id && a.gen == b.gen;
    }
    friend bool operator!=(const Entity& a, const Entity& b) noexcept {
        return !(a == b);
    }
};

class Registry {
public:
    Registry() = default;
    ~Registry() = default;

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) noexcept = default;
    Registry& operator=(Registry&&) noexcept = default;

    // ---- Entity lifecycle ----
    [[nodiscard]] Entity create();
    void destroy(Entity e);
    [[nodiscard]] bool valid(Entity e) const noexcept;
    void clear();

    // ---- Component operations ----
    template <class T, class... Args>
    T& emplace(Entity e, Args&&... args);

    template <class T>
    void remove(Entity e);

    template <class T>
    [[nodiscard]] bool has(Entity e) const;

    template <class T>
    [[nodiscard]] T& get(Entity e);

    template <class T>
    [[nodiscard]] const T& get(Entity e) const;

    template <class T>
    [[nodiscard]] T* try_get(Entity e) noexcept;

    template <class T>
    [[nodiscard]] const T* try_get(Entity e) const noexcept;

    // ---- Views ----
    template <class... Cs>
    class View;

    template <class... Cs>
    [[nodiscard]] View<Cs...> view();

    template <class... Cs>
    [[nodiscard]] View<const Cs...> view() const;

private:
    // ---- Pool base (type erasure) ----
    struct IPool {
        virtual ~IPool() = default;
        virtual void remove_entity(Entity::id_type id) = 0;
        virtual bool has_entity(Entity::id_type id) const = 0;
        virtual std::size_t size() const noexcept = 0;
        virtual const std::vector<Entity::id_type>& dense_entities() const noexcept = 0;
    };

    template <class T>
    struct Pool final : IPool {
        // Sparse set:
        // dense_entities_[i] is the entity id for component at dense_data_[i]
        // sparse_[entity_id] is index+1 into dense arrays, or 0 if not present
        std::vector<T> dense_data_;
        std::vector<Entity::id_type> dense_entities_;
        std::vector<std::uint32_t> sparse_; // index+1, 0 = missing

        void ensure_sparse(Entity::id_type id) {
            if (id >= sparse_.size()) sparse_.resize(static_cast<std::size_t>(id) + 1u, 0u);
        }

        template <class... Args>
        T& emplace(Entity::id_type id, Args&&... args) {
            ensure_sparse(id);
            if (sparse_[id] != 0u) {
                // Already exists: overwrite in-place (common ECS behavior).
                auto& existing = dense_data_[static_cast<std::size_t>(sparse_[id] - 1u)];
                existing = T(std::forward<Args>(args)...);
                return existing;
            }

            const auto idx = static_cast<std::uint32_t>(dense_data_.size());
            dense_data_.emplace_back(std::forward<Args>(args)...);
            dense_entities_.push_back(id);
            sparse_[id] = idx + 1u;
            return dense_data_.back();
        }

        void remove_entity(Entity::id_type id) override {
            if (id >= sparse_.size()) return;
            const auto sparse_val = sparse_[id];
            if (sparse_val == 0u) return;

            const std::uint32_t idx = sparse_val - 1u;
            const std::uint32_t last = static_cast<std::uint32_t>(dense_data_.size() - 1u);

            if (idx != last) {
                // Swap-remove
                dense_data_[idx] = std::move(dense_data_[last]);
                const auto moved_entity = dense_entities_[static_cast<std::size_t>(last)];
                dense_entities_[idx] = moved_entity;
                sparse_[moved_entity] = idx + 1u;
            }

            dense_data_.pop_back();
            dense_entities_.pop_back();
            sparse_[id] = 0u;
        }

        bool has_entity(Entity::id_type id) const override {
            return id < sparse_.size() && sparse_[id] != 0u;
        }

        std::size_t size() const noexcept override {
            return dense_data_.size();
        }

        const std::vector<Entity::id_type>& dense_entities() const noexcept override {
            return dense_entities_;
        }

        T* try_get(Entity::id_type id) noexcept {
            if (!has_entity(id)) return nullptr;
            return &dense_data_[static_cast<std::size_t>(sparse_[id] - 1u)];
        }

        const T* try_get(Entity::id_type id) const noexcept {
            if (!has_entity(id)) return nullptr;
            return &dense_data_[static_cast<std::size_t>(sparse_[id] - 1u)];
        }
    };

    template <class T>
    Pool<std::remove_const_t<T>>* pool_ptr() noexcept;

    template <class T>
    const Pool<std::remove_const_t<T>>* pool_ptr() const noexcept;

    template <class T>
    Pool<std::remove_const_t<T>>& ensure_pool();

private:
    // entity generations: gen_[id] is current generation for that id
    std::vector<Entity::gen_type> gen_;
    // alive flag per id
    std::vector<std::uint8_t> alive_;
    // recycled ids
    std::vector<Entity::id_type> free_ids_;

    // component pools by type
    std::unordered_map<std::type_index, std::unique_ptr<IPool>> pools_;

public:
    // ---- View implementation ----
    template <class... Cs>
    class View {
    public:
        using registry_type = std::conditional_t<
            (std::is_const_v<Cs> || ...),
            const Registry,
            Registry
        >;

        explicit View(registry_type& r) : reg_(r) {}

        template <class Fn>
        void each(Fn&& fn) {
            // Choose smallest pool among Cs...
            auto* driving = smallest_pool();
            if (!driving) return;

            const auto& dense_ids = driving->dense_entities();

            for (auto id : dense_ids) {
                Entity e{ id, reg_.gen_[id] };
                if (!reg_.alive_[id]) continue;
                if (!reg_.valid(e)) continue; // generation safety

                if (!(reg_.template has<std::remove_const_t<Cs>>(e) && ...)) continue;

                // Fetch references in component order Cs...
                std::invoke(
                    std::forward<Fn>(fn),
                    e,
                    reg_.template get<std::remove_const_t<Cs>>(e)...
                );
            }
        }

    private:
        registry_type& reg_;

        const IPool* smallest_pool() const {
            const IPool* best = nullptr;
            std::size_t best_size = std::numeric_limits<std::size_t>::max();

            // If any pool missing, view is empty
            auto check = [&](auto* p) {
                if (!p) return false;
                const auto s = p->size();
                if (s < best_size) {
                    best_size = s;
                    best = p;
                }
                return true;
            };

            // Fold over component types
            bool ok = (check(reg_.template pool_ptr<std::remove_const_t<Cs>>()) && ...);
            if (!ok) return nullptr;
            return best;
        }
    };
};

// ====================== Template implementations ======================

template <class T>
inline typename Registry::template Pool<std::remove_const_t<T>>* Registry::pool_ptr() noexcept {
    auto it = pools_.find(std::type_index(typeid(std::remove_const_t<T>)));
    if (it == pools_.end()) return nullptr;
    return static_cast<Pool<std::remove_const_t<T>>*>(it->second.get());
}

template <class T>
inline const typename Registry::template Pool<std::remove_const_t<T>>* Registry::pool_ptr() const noexcept {
    auto it = pools_.find(std::type_index(typeid(std::remove_const_t<T>)));
    if (it == pools_.end()) return nullptr;
    return static_cast<const Pool<std::remove_const_t<T>>*>(it->second.get());
}

template <class T>
inline typename Registry::template Pool<std::remove_const_t<T>>& Registry::ensure_pool() {
    using U = std::remove_const_t<T>;
    auto key = std::type_index(typeid(U));
    auto it = pools_.find(key);
    if (it == pools_.end()) {
        auto p = std::make_unique<Pool<U>>();
        auto* raw = p.get();
        pools_.emplace(key, std::move(p));
        return *raw;
    }
    return *static_cast<Pool<U>*>(it->second.get());
}

template <class T, class... Args>
inline T& Registry::emplace(Entity e, Args&&... args) {
    if (!valid(e)) throw std::runtime_error("Registry::emplace on invalid entity");
    auto& pool = ensure_pool<T>();
    return pool.emplace(e.id, std::forward<Args>(args)...);
}

template <class T>
inline void Registry::remove(Entity e) {
    if (!valid(e)) return;
    if (auto* p = pool_ptr<T>()) p->remove_entity(e.id);
}

template <class T>
inline bool Registry::has(Entity e) const {
    if (!valid(e)) return false;
    if (auto* p = pool_ptr<T>()) return p->has_entity(e.id);
    return false;
}

template <class T>
inline T& Registry::get(Entity e) {
    if (!valid(e)) throw std::runtime_error("Registry::get on invalid entity");
    auto* p = pool_ptr<T>();
    if (!p) throw std::runtime_error("Registry::get missing pool");
    auto* ptr = p->try_get(e.id);
    if (!ptr) throw std::runtime_error("Registry::get missing component");
    return *ptr;
}

template <class T>
inline const T& Registry::get(Entity e) const {
    if (!valid(e)) throw std::runtime_error("Registry::get(const) on invalid entity");
    auto* p = pool_ptr<T>();
    if (!p) throw std::runtime_error("Registry::get(const) missing pool");
    auto* ptr = p->try_get(e.id);
    if (!ptr) throw std::runtime_error("Registry::get(const) missing component");
    return *ptr;
}

template <class T>
inline T* Registry::try_get(Entity e) noexcept {
    if (!valid(e)) return nullptr;
    auto* p = pool_ptr<T>();
    return p ? p->try_get(e.id) : nullptr;
}

template <class T>
inline const T* Registry::try_get(Entity e) const noexcept {
    if (!valid(e)) return nullptr;
    auto* p = pool_ptr<T>();
    return p ? p->try_get(e.id) : nullptr;
}

template <class... Cs>
inline typename Registry::template View<Cs...> Registry::view() {
    return View<Cs...>(*this);
}

template <class... Cs>
inline typename Registry::template View<const Cs...> Registry::view() const {
    return View<const Cs...>(*this);
}

