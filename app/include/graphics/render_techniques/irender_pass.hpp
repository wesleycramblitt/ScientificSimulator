#pragma once

namespace exd {
namespace graphics {
namespace render_techniques {

// Base class for pass-style render techniques (particles, volumes, etc).
// Follows the same bind/draw/unbind lifecycle as IRenderTechnique.
class IRenderPass {
public:
    virtual ~IRenderPass() = default;
    virtual void bind()   = 0;
    virtual void unbind() = 0;
};

} // namespace render_techniques
} // namespace graphics
} // namespace exd
