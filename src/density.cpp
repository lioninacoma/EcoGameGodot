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
