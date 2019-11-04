#ifndef TEST_H
#define TEST_H

#include <Godot.hpp>
#include <Reference.hpp>

#include <string.h>
#include <vector>

using namespace std;

namespace godot {
	class Test : public Reference {
		GODOT_CLASS(Test, Reference)

	private:
		int x;

	public:
		static void _register_methods();
		
		Test();
		~Test();

		void _init(); // our initializer called by Godot

		int get_x();
		void set_x(int x);
	};
}

#endif