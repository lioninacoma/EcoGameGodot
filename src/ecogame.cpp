#include "ecogame.h"

using namespace godot;

void EcoGame::_register_methods() {
	register_method("buildChunk", &EcoGame::buildChunk);
	register_method("buildSections", &EcoGame::buildSections);
}

EcoGame::EcoGame() {
	chunkBuilder = new ChunkBuilder();
	sections = new Section * [SECTIONS_LEN * sizeof(*sections)];

	memset(sections, 0, SECTIONS_LEN * sizeof(*sections));
}

EcoGame::~EcoGame() {
	delete[] sections;
	delete chunkBuilder;
}

void EcoGame::_init() {
	// initialize any variables here
}

void EcoGame::buildSections(Vector3 pos, float d) {
	int xS, xE, zS, zE, x, z;
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	Section* section;
	Vector3 tl = Vector3(pos.x - d, 0, pos.z - d);
	Vector3 br = Vector3(pos.x + d, 0, pos.z + d);

	tl = fn::toSectionCoords(tl);
	br = fn::toSectionCoords(br);

	//Godot::print(String("tl: {0}, br: {1}").format(Array::make(tl, br)));

	xS = int(tl.x);
	xS = max(0, xS);

	xE = int(br.x);
	xE = min(SECTIONS_SIZE - 1, xE);

	zS = int(tl.z);
	zS = max(0, zS);

	zE = int(br.z);
	zE = min(SECTIONS_SIZE - 1, zE);

	for (z = zS; z <= zE; z++) {
		for (x = xS; x <= xE; x++) {
			if (sections[fn::fi2(x, z, SECTIONS_SIZE)]) continue;
			//Godot::print(String("section offset: {0}").format(Array::make(Vector2(x * SECTION_SIZE, z * SECTION_SIZE))));
			section = new Section(Vector2(x * SECTION_SIZE, z * SECTION_SIZE));
			sections[fn::fi2(x, z, SECTIONS_SIZE)] = section;
			ThreadPool::get()->submitTask(boost::bind(&EcoGame::buildSection, this, section, game));
		}
	}
}

void EcoGame::buildSection(Section* section, Node* game) {
	section->build(chunkBuilder, game);
}

void EcoGame::buildChunk(Variant vChunk) {
	Chunk* chunk = as<Chunk>(vChunk.operator Object * ());
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	chunkBuilder->build(chunk, game);
}
