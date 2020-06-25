#ifndef		HAS_DENSITY_H_BEEN_INCLUDED
#define		HAS_DENSITY_H_BEEN_INCLUDED

#include <Vector2.hpp>
#include <Vector3.hpp>

#include <algorithm> 

using namespace std;

float Sphere(const godot::Vector3& worldPosition, const godot::Vector3& origin, float radius);
float Cuboid(const godot::Vector3& worldPosition, const godot::Vector3& origin, const godot::Vector3& halfDimensions);
float Density_Func(const godot::Vector3& worldPosition);

#endif	//	HAS_DENSITY_H_BEEN_INCLUDED
