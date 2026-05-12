#pragma once

namespace combat {

// Auto-stop: перед выстрелом тормозит игрока через противоположные кнопки.
// Вызывается из MainThread каждый кадр.
void UpdateAutoStop(bool cs2Active);

} // namespace combat
