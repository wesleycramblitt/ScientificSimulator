#include "entities/entity.hpp"

class Registry {
public:
    Entity create();
    void destroy(Entity e);
    bool valid(Entity e) const;

    template<class T, class... Args>
    T& emplace(Entity e, Args&&... args);

    template<class T>
    void remove(Entity e);

    template<class T>
    bool has(Entity e) const;

    template<class T>
    T& get(Entity e);

    template<class... Ts>
    auto view(); // returns an iterable over entities with Ts...
};
