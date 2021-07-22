// Copyright (c) 2021 Paul Merrell
#include <iostream>
#include <fstream>
#include <sstream>
#include "parseTiledModel.h"

using namespace std;

// Parse a <tiledmodel /> input.
void parseTiledModel(InputSettings& settings) {
	string path = "samples/" + settings.name;
	ifstream modelFile(path, ios::in);
	if (!modelFile) {
		cout << "ERROR: The model file :" << path << " does not exist.\n" << endl;
		return;
	}

	char temp[1000];
	modelFile.getline(temp, 1000);
	modelFile.getline(temp, 1000);
	modelFile.getline(temp, 1000);

	// Read in the size and create the example model.
	int xSize, ySize, zSize;
	modelFile >> xSize;
	modelFile >> ySize;
	modelFile >> zSize;
	int*** example = new int** [xSize];
	for (int x = 0; x < xSize; x++) {
		example[x] = new int* [ySize];
		for (int y = 0; y < ySize; y++) {
			example[x][y] = new int[zSize];
		}
	}

	// Read in the labels in the example model.
	int buffer;
	modelFile >> buffer;
	for (int z = 0; z < zSize; z++) {
		for (int x = 0; x < xSize; x++) {
			for (int y = 0; y < ySize; y++) {
				example[x][y][z] = buffer;
				modelFile >> buffer;
			}
		}
	}

	// Find the number of labels in the model.
	int numLabels = 0;
	for (int x = 0; x < xSize; x++) {
		for (int y = 0; y < ySize; y++) {
			for (int z = 0; z < zSize; z++) {
				numLabels = max(numLabels, example[x][y][z]);
			}
		}
	}
	numLabels++;
	settings.numLabels = numLabels;

	// The transition describes which labels can be next to each other.
	// When transition[direction][labelA][labelB] is true that means labelA
	// can be just below labelB in the specified direction where x = 0, y = 1, z = 2.
	bool*** transition = createTransition(numLabels);
	settings.transition = transition;

	// Compute the transition.
	for (int x = 0; x < xSize - 1; x++) {
		for (int y = 0; y < ySize; y++) {
			for (int z = 0; z < zSize; z++) {
				int labelA = example[x][y][z];
				int labelB = example[x + 1][y][z];
				transition[0][labelA][labelB] = true;
			}
		}
	}
	for (int x = 0; x < xSize; x++) {
		for (int y = 0; y < ySize - 1; y++) {
			for (int z = 0; z < zSize; z++) {
				int labelA = example[x][y][z];
				int labelB = example[x][y + 1][z];
				transition[1][labelA][labelB] = true;
			}
		}
	}
	for (int x = 0; x < xSize; x++) {
		for (int y = 0; y < ySize; y++) {
			for (int z = 0; z < zSize - 1; z++) {
				int labelA = example[x][y][z];
				int labelB = example[x][y][z + 1];
				transition[2][labelA][labelB] = true;
			}
		}
	}

	// The number of labels of each type in the model.
	int* labelCount = new int[numLabels];
	for (int i = 0; i < numLabels; i++) {
		labelCount[i] = 0;
	}
	for (int x = 0; x < xSize; x++) {
		for (int y = 0; y < ySize; y++) {
			for (int z = 0; z < zSize; z++) {
				labelCount[example[x][y][z]]++;
			}
		}
	}

	// We could use the label count, but equal weight also works.
	for (int i = 0; i < numLabels; i++) {
		settings.weights.push_back(1.0);
	}

	// Find the label that should initially appear at the very bottom.
	// And find the ground plane label.
	int bottomLabel = -1;
	int groundLabel = -1;

	int* onBottom = new int[numLabels];
	for (int i = 0; i < numLabels; i++) {
		onBottom[i] = 0;
	}
	for (int x = 0; x < xSize; x++) {
		for (int y = 0; y < ySize; y++) {
			onBottom[example[x][y][0]]++;
		}
	}
	// The bottom and ground labels should be tileable and appear frequently.
	int bottomCount = 0;
	for (int i = 0; i < numLabels; i++) {
		if (transition[0][i][i] && transition[1][i][i] && (onBottom[i] > bottomCount)) {
			bottomLabel = i;
			bottomCount = onBottom[i];
		}
	}
	if (bottomLabel != -1) {
		int groundCount = 0;
		for (int i = 0; i < numLabels; i++) {
			if (transition[0][i][i] && transition[1][i][i] && transition[2][bottomLabel][i] &&
				transition[2][i][0] && (labelCount[i] > groundCount)) {
				groundLabel = i;
				groundCount = labelCount[i];
			}
		}
	}
	if (groundLabel == -1 || bottomLabel == -1) {
		bool modifyInBlocks = (settings.blockSize[0] < settings.size[0] || settings.blockSize[1] < settings.size[1]);
		if (modifyInBlocks) {
			cout << "The example model has no tileable ground plane. The new model can not be modified in blocks.\n" << endl;
		}
	}

	// Set the initial labels.
	settings.initialLabels = new int[settings.size[2]];
	settings.initialLabels[0] = bottomLabel;
	settings.initialLabels[1] = groundLabel;
	for (int z = 0; z < settings.size[2]; z++) {
		if (z < zSize) {
			settings.initialLabels[z] = example[0][0][z];
		} else {
			settings.initialLabels[z] = 0;
		}
	}

	// Delete the example model.
	for (int x = 0; x < xSize; x++) {
		for (int y = 0; y < ySize; y++) {
			delete example[x][y];
		}
		delete example[x];
	}
	delete example;

	stringstream stream;
	stream << modelFile.rdbuf();
	settings.tiledModelSuffix += stream.str();
}
