#include "density.h"

using namespace godot;

// ----------------------------------------------------------------------------

float Sphere(const Vector3& worldPosition, const Vector3& origin, float radius)
{
	return (worldPosition - origin).length() - radius;
}

// ----------------------------------------------------------------------------

float Cuboid(const Vector3& worldPosition, const Vector3& origin, const Vector3& halfDimensions)
{
	const Vector3& local_pos = worldPosition - origin;
	const Vector3& pos = local_pos;

	const Vector3& d = pos.abs() - halfDimensions;
	const float m = max(d.x, max(d.y, d.z));
	return min(m, d.length());
}

// ----------------------------------------------------------------------------

float Density_Func(const Vector3& worldPosition)
{
	const float MAX_HEIGHT = 20.f;
	const float terrain = worldPosition.y - MAX_HEIGHT;
	const float sphere = Sphere(worldPosition, Vector3(15.f, 2.5f, 1.f), 16.f);

	return min(sphere, terrain);
}
