#pragma once

// Forward declarations
struct ImDrawList;

namespace esp {

// Render out-of-FOV arrows pointing to enemies off-screen
void RenderOofArrows(ImDrawList* drawList);

} // namespace esp
