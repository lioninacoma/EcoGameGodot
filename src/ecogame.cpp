#include "ecogame.h"

using namespace godot;

void EcoGame::_register_methods() {
	register_method("buildChunk", &EcoGame::buildChunk);
}

EcoGame::EcoGame() {
	EcoGame::chunkBuilder;
}

EcoGame::~EcoGame() {
	// add your cleanup here
}

void EcoGame::_init() {
	// initialize any variables here
}

void EcoGame::buildChunk(Variant vChunk) {
	Chunk* chunk = as<Chunk>(vChunk.operator Object * ());
	Node* game = get_tree()->get_root()->get_node("EcoGame");

	chunkBuilder.build(chunk, game);
}

//void EcoGame::test(Variant v) {
//	Test* test = as<Test>(v.operator Object * ());
//	Godot::print(String("test x: {0}").format(Array::make(test->get_x())));
//}
