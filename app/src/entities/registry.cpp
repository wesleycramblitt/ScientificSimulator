
#include "entities/registry.hpp"


Entity Registry::create() {
    Entity::id_type id;

    if (!free_ids_.empty()) {
        id = free_ids_.back();
        free_ids_.pop_back();
        alive_[id] = 1u;
        // generation already incremented in destroy()
    } else {
        id = static_cast<Entity::id_type>(gen_.size());
        gen_.push_back(0u);
        alive_.push_back(1u);
    }

    return Entity{ id, gen_[id] };
}

void Registry::destroy(Entity e) {
    if (!valid(e)) return;

    const auto id = e.id;

    // Remove from all pools
    for (auto& [_, pool] : pools_) {
        pool->remove_entity(id);
    }

    // Invalidate entity by bumping generation, mark dead, recycle id
    alive_[id] = 0u;
    gen_[id] += 1u;
    free_ids_.push_back(id);
}

bool Registry::valid(Entity e) const noexcept {
    if (e.id >= gen_.size()) return false;
    if (!alive_[e.id]) return false;
    return gen_[e.id] == e.gen;
}

void Registry::clear() {
    // Clear pools
    pools_.clear();

    // Clear entities
    gen_.clear();
    alive_.clear();
    free_ids_.clear();
}

