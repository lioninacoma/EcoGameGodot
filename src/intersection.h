#ifndef INTERSECTION_H
#define INTERSECTION_H

#include <Vector3.hpp>
#include <Rect2.hpp>

#include <boost/function.hpp>

#include <vector>
#include <iostream>

#include "fn.h"

using namespace std;

namespace godot {

	class Intersection {
	public:
		template<class P>
		static vector<P> get(Vector3 start, Vector3 end, bool first, boost::function<P(int, int, int)> intersection, vector<P> list) {
			int it = 0, itMax = 32000;

			int gx0idx = floor(start.x);
			int gy0idx = floor(start.y);
			int gz0idx = floor(start.z);

			int gx1idx = floor(end.x);
			int gy1idx = floor(end.y);
			int gz1idx = floor(end.z);

			int sx = gx1idx > gx0idx ? 1 : (gx1idx == gx0idx ? 0 : - 1);
			int sy = gy1idx > gy0idx ? 1 : (gy1idx == gy0idx ? 0 : - 1);
			int sz = gz1idx > gz0idx ? 1 : (gz1idx == gz0idx ? 0 : - 1);

			int gx = gx0idx;
			int gy = gy0idx;
			int gz = gz0idx;

			// Planes for each axis that we will next cross
			int gxp = gx0idx + (gx1idx > gx0idx ? 1 : 0);
			int gyp = gy0idx + (gy1idx > gy0idx ? 1 : 0);
			int gzp = gz0idx + (gz1idx > gz0idx ? 1 : 0);

			// Only used for multiplying up the error margins
			float vx = end.x == start.x ? 1.0 : end.x - start.x;
			float vy = end.y == start.y ? 1.0 : end.y - start.y;
			float vz = end.z == start.z ? 1.0 : end.z - start.z;

			// Error is normalized to vx * vy * vz so we only have to multiply up
			float vxvy = vx * vy;
			float vxvz = vx * vz;
			float vyvz = vy * vz;

			// Error from the next plane accumulators, scaled up by vx * vy * vz
			// gx0 + vx * rx == = gxp
			// vx * rx == = gxp - gx0
			// rx == = (gxp - gx0) / vx
			float errx = (gxp - start.x) * vyvz;
			float erry = (gyp - start.y) * vxvz;
			float errz = (gzp - start.z) * vxvy;

			float derrx = sx * vyvz;
			float derry = sy * vxvz;
			float derrz = sz * vxvy;

			while (true) {
				P p = intersection(gx, gy, gz);
				if (p && !fn::contains(list, p)) {
					list.push_back(p);
					if (first) return list;
				}

				if (gx == gx1idx && gy == gy1idx && gz == gz1idx) break;

				// Which plane do we cross first ?
				float xr = abs(errx);
				float yr = abs(erry);
				float zr = abs(errz);

				if (sx != 0 && (sy == 0 || xr < yr) && (sz == 0 || xr < zr)) {
					gx += sx;
					errx += derrx;
				} 
				else if (sy != 0 && (sz == 0 || yr < zr)) {
					gy += sy;
					erry += derry;
				}
				else if (sz != 0) {
					gz += sz;
					errz += derrz;
				}

				it++;
				if (it >= itMax) break;
			}

			return list;
		}

		static bool isPointInRectangle(Vector2 start, Vector2 end, Vector2 p) {
			Rect2 r = Rect2(start, Vector2(abs(end.x - start.x), abs(end.y - start.y)));
			//Godot::print(String("start: {0}, end: {1}, p: {2}, r: {3}, area: {4}").format(Array::make(start, end, p, r, r.get_area())));
			return r.has_point(p);
		}
	};

}

#endif