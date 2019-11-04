#include "test.h"

using namespace godot;

void Test::_register_methods() {
	register_method("get_x", &Test::get_x);
	register_method("set_x", &Test::set_x);
}

Test::Test() {
	Test::x = 10;
}

Test::~Test() {

}

void Test::_init() {
	// initialize any variables here
}

int Test::get_x() {
	return Test::x;
}

void Test::set_x(int x) {
	Test::x = x;
}