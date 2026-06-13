#pragma once
#include "icomponent.hpp"

namespace exd {
namespace components {

/// Tag component.  Entities carrying this tag are displayed read-only in the
/// entity editor: enable/disable checkboxes are hidden and parameter widgets
/// are replaced with plain text.  Camera and Grid entities use this to prevent
/// accidental edits.
struct ReadOnly : public IComponent {
};

} // namespace components
} // namespace exd
