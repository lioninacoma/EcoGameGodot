#ifndef FN_H
#define FN_H

#include <vector>
#include <memory>

#include <boost/container_hash/hash.hpp>

#include "constants.h"

namespace godot {

	namespace fn {
		template <class T>
		static bool contains(const std::vector<T>& vec, const T& value) {
			return std::find(vec.begin(), vec.end(), value) != vec.end();
		}

		static int fi3(int x, int y, int z, int w, int h) {
			return x + w * (y + h * z);
		}

		static int fi3(int x, int y, int z) {
			return fi3(x, y, z, CHUNK_SIZE_X, CHUNK_SIZE_Y);
		}

		static int fi2(int x, int z, int w) {
			return x + z * w;
		}

		static int fi2(int x, int z) {
			return fi2(x, z, CHUNK_SIZE_X);
		}

		static Vector3 unreference(std::shared_ptr<Vector3> point) {
			return Vector3(point->x, point->y, point->z);
		}

		static void handle_eptr(std::exception_ptr eptr) {
			try {
				if (eptr) {
					std::rethrow_exception(eptr);
				}
			}
			catch (const std::exception & e) {
				std::cout << "Caught exception \"" << e.what() << "\"\n";
			}
		}

		static Vector3 toChunkCoords(Vector3 position) {
			int ix = (int)position.x;
			int iy = (int)position.y;
			int iz = (int)position.z;
			int chunkX = ix / CHUNK_SIZE_X;
			int chunkY = iy / CHUNK_SIZE_Y;
			int chunkZ = iz / CHUNK_SIZE_Z;
			position.x = chunkX;
			position.y = chunkY;
			position.z = chunkZ;
			return position;
		}

		static Vector2 toChunkCoords(Vector2 position) {
			Vector3 chunkCoords = toChunkCoords(Vector3(position.x, 0, position.y));
			return Vector2(chunkCoords.x, chunkCoords.z);
		}

		static std::size_t hash(Vector3 v) {
			std::size_t seed = 0;
			boost::hash_combine(seed, v.x);
			boost::hash_combine(seed, v.y);
			boost::hash_combine(seed, v.z);
			return seed;
		}

		static std::size_t hash(Vector2 v) {
			std::size_t seed = 0;
			boost::hash_combine(seed, v.x);
			boost::hash_combine(seed, v.y);
			return seed;
		}

		static float manhattan(Vector3 a, Vector3 b) {
			return abs(a.x - b.x) + abs(a.y - b.y) + abs(a.z - b.z);
		}

		static float euclidean(Vector3 a, Vector3 b) {
			return a.distance_to(b);
		}

		static Vector3 addNormal(Vector3 src, Vector3 normal) {
			return (src + normal).normalized();
		}
	}
}

#endif