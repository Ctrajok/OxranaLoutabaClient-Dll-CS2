#include "ui_menu.hpp"
#include "../config/config_vars.hpp"

namespace ui {

void RenderMenu()
{
	// Placeholder - menu rendering not extracted yet (complex logic)
}

bool IsMenuOpen()
{
	return config::g_menuOpen;
}

void ToggleMenu()
{
	config::g_menuOpen = !config::g_menuOpen;
}

} // namespace ui
