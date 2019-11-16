#ifndef ECOGAME_H
#define ECOGAME_H

#include <Godot.hpp>
#include <SceneTree.hpp>
#include <ViewPort.hpp>
#include <Node.hpp>
#include <Node.hpp>
#include <String.hpp>
#include <Array.hpp>

#include "constants.h"
#include "chunkbuilder.h"

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

		void _init(); // our initializer called by Godot
		void buildChunk(Variant vChunk);
	};

}

#endif
