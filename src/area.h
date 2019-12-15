#ifndef AREA_H
#define AREA_H

#include <Vector2.hpp>

namespace godot {

	class Area {
	private:
		Vector2 start;
		Vector2 end;
		Vector2 offset;
		float y;
	public:
		Area() : Area(Vector2(0, 0), Vector2(0, 0), 0) {};
		Area(Vector2 start, Vector2 end, float y) {
			Area::start = start;
			Area::end = end;
			Area::y = y;
		}

		void setOffset(Vector2 offset) {
			Area::offset = offset;
		}

		Vector2 getOffset() {
			return offset;
		}

		Vector2 getStart() {
			return Area::start;
		}

		Vector2 getEnd() {
			return Area::end;
		}

		float getY() {
			return Area::y;
		}

		float getWidth() {
			return (Area::end.x - Area::start.x);
		}

		float getHeight() {
			return (Area::end.y - Area::start.y);
		}
	};

}

#endif