// Copyright (c) 2021 Paul Merrell
#include "PropagatorAc3.h"

PropagatorAc3::PropagatorAc3(
	InputSettings* newSettings,
	int* newPossibilitySize,
	int* newOffset
) : Propagator(newSettings) {
	settings = newSettings;
	possibilitySize = newPossibilitySize;
	offset = newOffset;
	numLabels = settings->numLabels;
	size = settings->size;

	possibleLabels = new bool*** [possibilitySize[0]];
	inQueue = new bool** [possibilitySize[0]];
	for (int x = 0; x < possibilitySize[0]; x++) {
		possibleLabels[x] = new bool** [possibilitySize[1]];
		inQueue[x] = new bool* [possibilitySize[1]];
		for (int y = 0; y < possibilitySize[1]; y++) {
			possibleLabels[x][y] = new bool* [possibilitySize[2]];
			inQueue[x][y] = new bool[possibilitySize[2]];
			for (int z = 0; z < possibilitySize[2]; z++) {
				possibleLabels[x][y][z] = new bool[numLabels];
			}
		}
	}
}

// Remove a label in the block at the given position.
bool PropagatorAc3::removeLabel(int label, int position[3]) {
	int x = position[0];
	int y = position[1];
	int z = position[2];
	if (!possibleLabels[x][y][z][label]) {
		return true;
	}

	possibleLabels[x][y][z][label] = false;
	inQueue[x][y][z] = true;

	// ***************************
	// TODO: Combine this with setBlockLabel.
	// ***************************
	std::deque<int*> updateQueue;
	updateQueue.push_back(position);
	inQueue[x][y][z] = true;
	while (updateQueue.size() > 0) {
		int* update = updateQueue.front();
		int x = update[0];
		int y = update[1];
		int z = update[2];

		// TODO: Check if this makes things faster or not.
		// Check if any possible labels are still left.
		// If not we have failed.
		bool isPossible = false;
		for (int i = 0; i < numLabels; i++) {
			if (possibleLabels[x][y][z][i]) {
				isPossible = true;
				break;
			}
		}
		if (!isPossible) {
			return false;
		}
		for (int dir = 0; dir < 6; dir++) {
			propagate(x, y, z, dir, updateQueue);
		}
		inQueue[x][y][z] = false;
		updateQueue.pop_front();
	}
	return true;
}

// Set a label in the block at the given position.
bool PropagatorAc3::setBlockLabel(int label, int position[3]) {
	int x = position[0];
	int y = position[1];
	int z = position[2];
	for (int i = 0; i < numLabels; i++) {
		possibleLabels[x][y][z][i] = (i == label);
	}

	std::deque<int*> updateQueue;
	updateQueue.push_back(position);
	inQueue[x][y][z] = true;
	while (updateQueue.size() > 0) {
		int* update = updateQueue.front();
		int x = update[0];
		int y = update[1];
		int z = update[2];

		// TODO: Check if this makes things faster or not.
		// Check if any possible labels are still left.
		// If not we have failed.
		bool isPossible = false;
		for (int i = 0; i < numLabels; i++) {
			if (possibleLabels[x][y][z][i]) {
				isPossible = true;
				break;
			}
		}
		if (!isPossible) {
			return false;
		}
		for (int dir = 0; dir < 6; dir++) {
			propagate(x, y, z, dir, updateQueue);
		}
		inQueue[x][y][z] = false;
		updateQueue.pop_front();
	}
	return true;
}

// Set a label in the block at the given position.
void PropagatorAc3::resetBlock() {
	for (int x = 0; x < possibilitySize[0]; x++) {
		for (int y = 0; y < possibilitySize[1]; y++) {
			for (int z = 0; z < possibilitySize[2]; z++) {
				inQueue[x][y][z] = false;
				for (int label = 0; label < numLabels; label++) {
					possibleLabels[x][y][z][label] = true;
				}
			}
		}
	}
}

// Set a label in the block at the given position.
bool PropagatorAc3::isPossible(int x, int y, int z, int label) {
	return possibleLabels[x][y][z][label];
}

void PropagatorAc3::propagate(int xB, int yB, int zB, int dir, std::deque<int*>& updateQueue) {
	bool*** transition = settings->transition;

	int xA = xB;
	int yA = yB;
	int zA = zB;
	switch (dir) {
	case 0: xA--; break;
	case 1: xA++; break;
	case 2: yA--; break;
	case 3: yA++; break;
	case 4: zA--; break;
	case 5: zA++; break;
	}
	// Do not propagate if this goes outside the bounds of the block.
	if (settings->periodic) {
		switch (dir) {
		case 0: if (xA < offset[0]) { xA += size[0]; } break;
		case 2: if (yA < offset[1]) { yA += size[1]; } break;
		case 4: if (zA <= offset[2]) { return; } break;
		case 1: if (xA > possibilitySize[0] - offset[0] - 1) { xA -= size[0]; } break;
		case 3: if (yA > possibilitySize[1] - offset[1] - 1) { yA -= size[1]; } break;
		case 5: if (zA > possibilitySize[2] - offset[2] - 1) { return; } break;
		}
	}
	else {
		switch (dir) {
		case 0: if (xB <= offset[0]) { return; } break;
		case 2: if (yB <= offset[1]) { return; } break;
		case 4: if (zB <= offset[2]) { return; } break;
		case 1: if (xB >= possibilitySize[0] - offset[0] - 1) { return; } break;
		case 3: if (yB >= possibilitySize[1] - offset[1] - 1) { return; } break;
		case 5: if (zB >= possibilitySize[2] - offset[2] - 1) { return; } break;
		}
	}

	int dim = dir / 2;
	bool positive = (dir % 2 == 1);
	for (int a = 0; a < numLabels; a++) {
		if (possibleLabels[xA][yA][zA][a]) {
			bool acceptable = false;
			for (int b = 0; b < numLabels; b++) {
				bool validTransition = positive ? transition[dim][b][a] : transition[dim][a][b];
				if (validTransition && possibleLabels[xB][yB][zB][b]) {
					acceptable = true;
					break;
				}
			}
			if (!acceptable) {
				possibleLabels[xA][yA][zA][a] = false;
				if (!inQueue[xA][yA][zA]) {
					int* posA = new int[3];
					posA[0] = xA;
					posA[1] = yA;
					posA[2] = zA;
					updateQueue.push_back(posA);
				}
			}
		}
	}
}
