#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <string>

// Config module: Configuration save/load management
// This file contains functions for persisting configuration to disk

namespace config {

// Save current configuration to INI file
void SaveConfig();

// Load configuration from INI file
void LoadConfig();

} // namespace config

#endif // CONFIG_MANAGER_HPP
