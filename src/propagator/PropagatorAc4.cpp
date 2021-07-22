// Copyright (c) 2021 Paul Merrell
#include "PropagatorAc4.h"
#include <vector>
#include <iostream>

PropagatorAc4::PropagatorAc4(
	InputSettings* newSettings,
	int* newPossibilitySize,
	int* newOffset
) : Propagator(newSettings) {
	settings = newSettings;
	possibilitySize = newPossibilitySize;
	offset = newOffset;
	numLabels = settings->numLabels;
	size = settings->size;
	numDirections = 2 * settings->numDims;

	possibleLabels = new bool*** [possibilitySize[0]];
	for (int x = 0; x < possibilitySize[0]; x++) {
		possibleLabels[x] = new bool** [possibilitySize[1]];
		for (int y = 0; y < possibilitySize[1]; y++) {
			possibleLabels[x][y] = new bool* [possibilitySize[2]];
			for (int z = 0; z < possibilitySize[2]; z++) {
				possibleLabels[x][y][z] = new bool[numLabels];
			}
		}
	}
	support = new int**** [possibilitySize[0]];
	for (int x = 0; x < possibilitySize[0]; x++) {
		support[x] = new int*** [possibilitySize[1]];
		for (int y = 0; y < possibilitySize[1]; y++) {
			support[x][y] = new int** [possibilitySize[2]];
			for (int z = 0; z < possibilitySize[2]; z++) {
				support[x][y][z] = new int* [numLabels];
				for (int i = 0; i < numLabels; i++) {
					support[x][y][z][i] = new int[numDirections];
				}
			}
		}
	}
}

void addToQueue(int x, int y, int z, int label, std::deque<vector<int>>& updateQueue) {
	vector<int> labeledPos(4);
	labeledPos[0] = x;
	labeledPos[1] = y;
	labeledPos[2] = z;
	labeledPos[3] = label;
	updateQueue.push_back(labeledPos);
}

// Propagate everything in the update queue.
void PropagatorAc4::propagate(deque<vector<int>>& updateQueue) {
	while (updateQueue.size() > 0) {
		vector<int> update = updateQueue.front();
		int xC = update[0];
		int yC = update[1];
		int zC = update[2];

		vector<vector<int>> cSupporting = settings->supporting[update[3]];
		for (int dir = 0; dir < numDirections; dir++) {
			int xB = xC;
			int yB = yC;
			int zB = zC;
			switch (dir) {
			case 0: xB--; break;
			case 1: xB++; break;
			case 2: yB--; break;
			case 3: yB++; break;
			case 4: zB--; break;
			case 5: zB++; break;
			}
			// Do not propagate if this goes outside the bounds of the block.
			if (settings->periodic) {
				switch (dir) {
				case 0: if (xB < offset[0]) { xB += size[0]; } break;
				case 2: if (yB < offset[1]) { yB += size[1]; } break;
				case 4: if (zB <= offset[2]) { continue; } break;
				case 1: if (xB > possibilitySize[0] - offset[0] - 1) { xB -= size[0]; } break;
				case 3: if (yB > possibilitySize[1] - offset[1] - 1) { yB -= size[1]; } break;
				case 5: if (zB > possibilitySize[2] - offset[2] - 1) { continue; } break;
				}
			}
			else {
				switch (dir) {
				case 0: if (xC <= offset[0]) { continue; } break;
				case 2: if (yC <= offset[1]) { continue; } break;
				case 4: if (zC <= offset[2]) { continue; } break;
				case 1: if (xC >= possibilitySize[0] - offset[0] - 1) { continue; } break;
				case 3: if (yC >= possibilitySize[1] - offset[1] - 1) { continue; } break;
				case 5: if (zC >= possibilitySize[2] - offset[2] - 1) { continue; } break;
				}
			}
			vector<int> dirSupporting = cSupporting[dir];
			for (int i = 0; i < (int)dirSupporting.size(); i++) {
				int b = dirSupporting[i];
				support[xB][yB][zB][b][dir]--;
				if (support[xB][yB][zB][b][dir] == 0 && possibleLabels[xB][yB][zB][b]) {
					possibleLabels[xB][yB][zB][b] = false;
					addToQueue(xB, yB, zB, b, updateQueue);
				}
			}
		}
		updateQueue.pop_front();
	}
}

// Set a label in the block at the given position.
bool PropagatorAc4::setBlockLabel(int label, int position[3]) {
	std::deque<vector<int>> updateQueue;
	int x = position[0];
	int y = position[1];
	int z = position[2];
	for (int i = 0; i < numLabels; i++) {
		if (i != label && possibleLabels[x][y][z][i]) {
			vector<int> labeledPos(4);
			possibleLabels[x][y][z][i] = false;
			labeledPos[0] = x;
			labeledPos[1] = y;
			labeledPos[2] = z;
			labeledPos[3] = i;
			updateQueue.push_back(labeledPos);
		}
	}
	propagate(updateQueue);
	return true;
}

// Remove a label in the block at the given position.
bool PropagatorAc4::removeLabel(int label, int position[3]) {
	int x = position[0];
	int y = position[1];
	int z = position[2];
	possibleLabels[x][y][z][label] = false;
	vector<int> labeledPos(4);
	labeledPos[0] = x;
	labeledPos[1] = y;
	labeledPos[2] = z;
	labeledPos[3] = label;
	std::deque<vector<int>> updateQueue;
	updateQueue.push_back(labeledPos);
	propagate(updateQueue);
	return true;
}

// Set a label in the block at the given position.
void PropagatorAc4::resetBlock() {
	for (int x = 0; x < possibilitySize[0]; x++) {
		for (int y = 0; y < possibilitySize[1]; y++) {
			for (int z = 0; z < possibilitySize[2]; z++) {
				for (int label = 0; label < numLabels; label++) {
					possibleLabels[x][y][z][label] = true;
					for (int dir = 0; dir < numDirections; dir++) {
						support[x][y][z][label][dir] = settings->supportCount[label][dir];
					}
				}
			}
		}
	}
}

// Set a label in the block at the given position.
bool PropagatorAc4::isPossible(int x, int y, int z, int label) {
	return possibleLabels[x][y][z][label];
}
