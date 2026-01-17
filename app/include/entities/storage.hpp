#include "entities/entity.hpp"
#include <span>

template<class T>
class Storage {
    public:
        void emplace(Entity e, T value);
        void remove(Entity e);
        bool has(Entity e) const;
        T& get(Entity e);

        std::span<const Entity> entities() const;
        std::span<T> data();
}
