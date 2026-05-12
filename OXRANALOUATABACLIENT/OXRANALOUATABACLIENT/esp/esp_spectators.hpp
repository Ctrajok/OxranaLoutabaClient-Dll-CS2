#pragma once

#include <vector>
#include <string>

namespace esp {

// Get spectators list accessor
std::vector<std::string>& GetSpectators();

// Render spectator list window
void RenderSpectatorList();

} // namespace esp
