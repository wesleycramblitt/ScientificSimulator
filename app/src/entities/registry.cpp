#include "entities/registry.hpp"

Entity Registry::create(std::string name) {
    Entity::id_type id;

    if (!free_ids_.empty()) {
        id = free_ids_.back();
        free_ids_.pop_back();
        alive_[id] = 1u;
        // gen_[id] already holds current generation for this id
    } else {
        id = static_cast<Entity::id_type>(gen_.size());
        gen_.push_back(0);
        alive_.push_back(1u);
    }

    names_.push_back(name);

    return Entity{id, gen_[id], name};
}

bool Registry::valid(Entity e) const noexcept {
    if (e.id == std::numeric_limits<Entity::id_type>::max()) return false;
    if (e.id >= gen_.size()) return false;
    if (e.id >= alive_.size()) return false;
    if (!alive_[e.id]) return false;
    return gen_[e.id] == e.gen;
}

void Registry::destroy(Entity e) {
    if (!valid(e)) return;

    // Mark dead and bump generation so stale handles become invalid
    alive_[e.id] = 0u;
    ++gen_[e.id];
    names_[e.id] = "";

    // Remove all components for this entity id from every pool.
    // (This is why we keep IPool type erasure.)
    for (auto& [_, pool] : pools_) {
        pool->remove_entity(e.id);
    }

    // Recycle id
    free_ids_.push_back(e.id);
}

void Registry::clear() {
    // Destroy all entities (without relying on external handles).
    // Increment gen for all alive entities and clear components.
    for (Entity::id_type id = 0; id < alive_.size(); ++id) {
        if (alive_[id]) {
            alive_[id] = 0u;
            ++gen_[id];
            names_[id] = "";
            free_ids_.push_back(id);
        }
    }

    for (auto& [_, pool] : pools_) {
        // brute-force clear by removing every entity id present in pool's dense list
        // (copy ids because remove mutates dense list)
        const auto ids = pool->dense_entities();
        for (auto id : ids) {
            pool->remove_entity(id);
        }
    }
}

std::vector<Entity> Registry::all_entities() const noexcept {
    std::vector<Entity> result;
    for (Entity::id_type id = 0; id < alive_.size(); ++id) {
        if (alive_[id]) {
            result.push_back(Entity{id, gen_[id], names_[id]});
        }
    }
    return result;
}

std::size_t Registry::entity_count() const noexcept {
    std::size_t count = 0;
    for (Entity::id_type id = 0; id < alive_.size(); ++id) {
        if (alive_[id]) ++count;
    }
    return count;
}
