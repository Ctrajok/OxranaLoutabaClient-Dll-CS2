#include "skins_weapons.hpp"

namespace skins {

const char* GetWeaponName(int defIndex)
{
	switch(defIndex) {
		case WEAPON_DEAGLE: return "DEAGLE";
		case WEAPON_ELITE: return "ELITE";
		case WEAPON_FIVESEVEN: return "FIVESEVEN";
		case WEAPON_GLOCK: return "GLOCK";
		case WEAPON_AK47: return "AK-47";
		case WEAPON_AUG: return "AUG";
		case WEAPON_AWP: return "AWP";
		case WEAPON_FAMAS: return "FAMAS";
		case WEAPON_G3SG1: return "G3SG1";
		case WEAPON_GALIL: return "GALIL";
		case WEAPON_M249: return "M249";
		case WEAPON_M4A4: return "M4A4";
		case WEAPON_MAC10: return "MAC10";
		case WEAPON_P90: return "P90";
		case WEAPON_MP5: return "MP5";
		case WEAPON_UMP45: return "UMP45";
		case WEAPON_XM1014: return "XM1014";
		case WEAPON_BIZON: return "BIZON";
		case WEAPON_MAG7: return "MAG7";
		case WEAPON_NEGEV: return "NEGEV";
		case WEAPON_SAWEDOFF: return "SAWEDOFF";
		case WEAPON_TEC9: return "TEC9";
		case WEAPON_ZEUS: return "ZEUS";
		case WEAPON_P2000: return "P2000";
		case WEAPON_MP7: return "MP7";
		case WEAPON_MP9: return "MP9";
		case WEAPON_NOVA: return "NOVA";
		case WEAPON_P250: return "P250";
		case WEAPON_SCAR20: return "SCAR20";
		case WEAPON_SG556: return "SG556";
		case WEAPON_SCOUT: return "SCOUT";
		case WEAPON_KNIFE:
		case WEAPON_KNIFE2:
		case WEAPON_KNIFE3: return "KNIFE";
		case WEAPON_FLASH: return "FLASH";
		case WEAPON_HE: return "HE";
		case WEAPON_SMOKE: return "SMOKE";
		case WEAPON_MOLOTOV: return "MOLOTOV";
		case WEAPON_DECOY: return "DECOY";
		case WEAPON_INCENDIARY: return "INCENDIARY";
		case WEAPON_C4: return "C4";
		case WEAPON_M4A1S: return "M4A1-S";
		case WEAPON_USPS: return "USP-S";
		case WEAPON_CZ75: return "CZ75";
		case WEAPON_REVOLVER: return "REVOLVER";
		case WEAPON_BAYONET: return "BAYONET";
		case WEAPON_CLASSIC: return "CLASSIC";
		case WEAPON_FLIP: return "FLIP";
		case WEAPON_GUT: return "GUT";
		case WEAPON_KARAMBIT: return "KARAMBIT";
		case WEAPON_M9: return "M9";
		case WEAPON_HUNTSMAN: return "HUNTSMAN";
		case WEAPON_FALCHION: return "FALCHION";
		case WEAPON_BOWIE: return "BOWIE";
		case WEAPON_BUTTERFLY: return "BUTTERFLY";
		case WEAPON_DAGGERS: return "DAGGERS";
		default: return "WEAPON";
	}
}

} // namespace skins
