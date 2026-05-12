#ifndef HOOKS_MANAGER_HPP
#define HOOKS_MANAGER_HPP

// Hooks module: Hook management and initialization
// This file contains functions for installing and uninstalling all hooks

namespace hooks {

// Initialize all hooks (MinHook, VTable hooks, etc.)
void Install();

// Remove all hooks and restore original functions
void Uninstall();

} // namespace hooks

#endif // HOOKS_MANAGER_HPP
