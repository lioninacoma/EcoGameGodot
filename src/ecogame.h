#ifndef ECOGAME_H
#define ECOGAME_H

#include <Godot.hpp>
#include <Node.hpp>
#include <String.hpp>
#include <Array.hpp>

#include <iostream>

#include "constants.h"
#include "chunkbuilder.h"
#include "scenehandle.h"

using namespace std;

namespace godot {

	class EcoGame : public Node {
		GODOT_CLASS(EcoGame, Node)

	private:
		ChunkBuilder chunkBuilder;
	public:
		static void _register_methods();

		EcoGame();
		~EcoGame();

		void _init();
		void ready();

		void buildChunk(Variant vChunk);
	};

}

#endif
