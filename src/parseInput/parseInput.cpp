// Copyright (c) 2021 Paul Merrell
#include "parseInput.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <chrono>
#include "../third_party/lodepng/lodepng.h"
#include "parseOverlapping.h"
#include "parseSimpleTiled.h"
#include "parseTiledModel.h"

using namespace std;
using namespace std::chrono;

// Find the labels that support each label in each direction.
void computeSupport(InputSettings& settings) {
	int N = settings.numLabels;
	settings.supporting.resize(N);
	settings.supportCount.resize(N);
	bool*** transition = settings.transition;
	for (int c = 0; c < N; c++) {
		int numDirections = 2 * settings.numDims;
		std::vector<std::vector<int>> supportingC(numDirections);
		std::vector<int> supportCountC(numDirections);
		for (int dir = 0; dir < numDirections; dir++) {
			std::vector<int> supportingDir;
			int dim = dir / 2;
			bool sign = dir % 2 == 0;
			if (sign) {
				for (int b = 0; b < N; b++) {
					if (transition[dim][b][c]) {
						// b supports c in direction dir.
						supportingDir.push_back(b);
					}
				}
			}
			else {
				for (int b = 0; b < N; b++) {
					if (transition[dim][c][b]) {
						// b supports c in direction dir.
						supportingDir.push_back(b);
					}
				}
			}
			supportingC[dir] = supportingDir;
			supportCountC[dir ^ 1] = (int)supportingDir.size();
		}
		settings.supporting[c] = supportingC;
		settings.supportCount[c] = supportCountC;
	}
}

InputSettings* parseInput(const XMLNode& node, microseconds& inputTime) {
	auto startInput = high_resolution_clock::now();

	InputSettings* settings = new InputSettings();

	settings->name = node.getAttributeStr(L"name");
	settings->size[0] = parseInt(node, L"width", 0);
	settings->size[1] = parseInt(node, L"length", 0);
	settings->size[2] = parseInt(node, L"height", 0);
	settings->blockSize[0] = parseInt(node, L"blockWidth", 0);
	settings->blockSize[1] = parseInt(node, L"blockLength", 0);
	settings->blockSize[2] = parseInt(node, L"blockHeight", 0);
	settings->subset = node.getAttributeStr(L"subset");

	// Switch length and height if length is 0.
	if (settings->size[1] == 0) {
		int temp = settings->size[2];
		settings->size[2] = settings->size[1];
		settings->size[1] = temp;
	}
	settings->size[2] = max(settings->size[2], 1);

	// Compute the block size if not given or if too large.
	for (int dim = 0; dim < 3; dim++) {
		if (settings->blockSize[dim] > settings->size[dim]) {
			cout << "ERROR: The block size cannot be larger than the output size." << endl;
			settings->blockSize[dim] = settings->size[dim];
		}
		if (settings->blockSize[dim] == 0) {
			settings->blockSize[dim] = settings->size[dim];
		}
	}
	settings->periodic = parseBool(node, L"periodic", false);
	if (settings->periodic && (settings->blockSize[0] < settings->size[0] || settings->blockSize[1] < settings->size[1])) {
		cout << "Periodic not implemented when modifying in blocks." << endl;
	}

	settings->type = node.getNameStr();
	if (settings->type == "simpletiled") {
		settings->numDims = 2;
		parseSimpleTiled(*settings);
	} else if (settings->type == "overlapping") {
		settings->numDims = 2;
		settings->tileWidth = parseInt(node, L"N", 0);
		settings->tileHeight = settings->tileWidth;
		settings->periodicInput = parseBool(node, L"periodicInput", true);
		settings->symmetry = parseInt(node, L"symmetry", 8);
		bool hasGround = parseInt(node, L"ground", 1234) != 1234;
		settings->ground = hasGround ? 1 : -1;
		parseOverlapping(*settings);
	}
	else if (settings->type == "tiledmodel") {
		settings->numDims = 3;
		parseTiledModel(*settings);
	} else {
		cout << "ERROR: Only simpledtiled or tiledmodel are allowed." << endl;
	}
	if (settings->useAc4) {
		computeSupport(*settings);
	}

	auto endInput = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(endInput - startInput);
	inputTime += duration;
	return settings;
}

