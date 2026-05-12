#pragma once

namespace movement {

// Jump-Duck: по одной кнопке жмёт jump+duck одновременно.
// Для пролезания в проёмы, вентиляции, boost-позиции.
void UpdateJumpDuck(bool cs2Active);

// Jump Scout: автовыстрел в момент приземления (velocity.z → 0).
// Работает когда в руках SSG/AWP и зажат aim key.
void UpdateJumpScout(bool cs2Active);

} // namespace movement
