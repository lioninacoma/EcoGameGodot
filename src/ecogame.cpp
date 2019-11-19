#include "ecogame.h"
#include <iostream>
#define bool int  
#define R 6  
#define C 5  

using namespace std;

void printMaxSubSquare(bool M[R][C]) {
	int i, j;
	int S[R][C];
	int max_of_s, max_i, max_j;

	/* Set first column of S[][]*/
	for (i = 0; i < R; i++)
		S[i][0] = M[i][0];

	/* Set first row of S[][]*/
	for (j = 0; j < C; j++)
		S[0][j] = M[0][j];

	/* Construct other entries of S[][]*/
	for (i = 1; i < R; i++) {
		for (j = 1; j < C; j++) {
			if (M[i][j] == 1)
				S[i][j] = min(S[i][j - 1], min(S[i - 1][j],
					S[i - 1][j - 1])) + 1;
			else
				S[i][j] = 0;
		}
	}

	/* Find the maximum entry, and indexes of maximum entry
		in S[][] */
	max_of_s = S[0][0]; max_i = 0; max_j = 0;
	for (i = 0; i < R; i++) {
		for (j = 0; j < C; j++) {
			if (max_of_s < S[i][j]) {
				max_of_s = S[i][j];
				max_i = i;
				max_j = j;
			}
		}
	}

	cout << "Maximum size sub-matrix is: \n";
	for (i = max_i; i > max_i - max_of_s; i--) {
		for (j = max_j; j > max_j - max_of_s; j--) {
			cout << M[i][j] << " ";
		}
		cout << "\n";
	}
}

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
	bool M[R][C] = {{0, 1, 1, 0, 1},
					{1, 1, 0, 1, 0},
					{1, 1, 1, 1, 1},
					{1, 1, 1, 1, 1},
					{1, 1, 1, 1, 1},
					{1, 1, 1, 1, 0}};

	printMaxSubSquare(M);
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
