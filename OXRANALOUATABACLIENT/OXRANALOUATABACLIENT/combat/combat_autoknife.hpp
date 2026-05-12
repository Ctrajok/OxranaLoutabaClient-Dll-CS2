#pragma once

namespace combat {

// Auto-knife: когда в руках нож и враг в радиусе — автоматически бьёт.
// Если враг спиной — правая кнопка (backstab 180dmg), иначе левая (slash 40dmg).
void UpdateAutoKnife(bool cs2Active);

} // namespace combat
