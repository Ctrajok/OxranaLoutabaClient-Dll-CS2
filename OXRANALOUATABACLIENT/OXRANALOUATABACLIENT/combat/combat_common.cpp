#include "combat_common.hpp"
#include <cmath>
#include "../memory/memory_utils.hpp"
#include "../memory/memory_offsets.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Types still in main file - need full definitions
struct Vec3 { float x, y, z; };
struct QAngle { float x, y, z; };

namespace combat {

QAngle CalcAngle(Vec3 src, Vec3 dst)
{
	QAngle angles;
	// ИСПРАВЛЕНО: Цель минус Я (dst - src), а не наоборот
	Vec3 delta = { dst.x - src.x, dst.y - src.y, dst.z - src.z };
	
	// ИСПРАВЛЕНИЕ: Используем более точный расчет гипотенузы
	float hyp = std::sqrt(delta.x * delta.x + delta.y * delta.y);

	// ИСПРАВЛЕНИЕ: Pitch (вертикальный угол) - используем отрицательное значение для правильного направления
	angles.x = -std::atan2(delta.z, hyp) * (180.0f / M_PI);
	
	// ИСПРАВЛЕНИЕ: Yaw (горизонтальный угол) - стандартный расчет
	angles.y = std::atan2(delta.y, delta.x) * (180.0f / M_PI);
	
	angles.z = 0.0f;

	return angles;
}

float GetFov(QAngle viewAngles, QAngle aimAngles)
{
	float deltaX = aimAngles.x - viewAngles.x;
	float deltaY = aimAngles.y - viewAngles.y;

	if (deltaY > 180.0f) deltaY -= 360.0f;
	if (deltaY < -180.0f) deltaY += 360.0f;

	return std::sqrt(deltaX * deltaX + deltaY * deltaY);
}

void ClampAngles(QAngle& angles)
{
	if (angles.x > 89.0f) angles.x = 89.0f;
	if (angles.x < -89.0f) angles.x = -89.0f;
	while (angles.y > 180.0f) angles.y -= 360.0f;
	while (angles.y < -180.0f) angles.y += 360.0f;
	angles.z = 0.0f;
}

Vec3 GetBonePos(uintptr_t pawn, int boneIndex)
{
	Vec3 pos = { 0, 0, 0 };
	if (!pawn) return pos;

	// Получаем GameSceneNode
	uintptr_t gameScene = 0;
	if (!memory::TryRead<uintptr_t>(pawn + memory::GetOffsets().m_pGameSceneNode, gameScene) || !gameScene)
		return pos;

	// Получаем BoneArray (ModelState + 0x80)
	uintptr_t boneMatrix = 0;
	if (!memory::TryRead<uintptr_t>(gameScene + memory::GetOffsets().m_modelState + 0x80, boneMatrix) || !boneMatrix)
		return pos;

	// Читаем координаты напрямую из памяти
	float x = 0.0f, y = 0.0f, z = 0.0f;
	if (memory::TryRead<float>(boneMatrix + boneIndex * 0x20 + 0x0, x) &&
		memory::TryRead<float>(boneMatrix + boneIndex * 0x20 + 0x4, y) &&
		memory::TryRead<float>(boneMatrix + boneIndex * 0x20 + 0x8, z))
	{
		pos.x = x;
		pos.y = y;
		pos.z = z;
	}
	return pos;
}

} // namespace combat
