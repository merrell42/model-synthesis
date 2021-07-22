// Copyright (c) 2021 Paul Merrell
#include "synthesizer.h"
#include "propagator/PropagatorAc3.h"
#include "propagator/PropagatorAc4.h"
#include <deque>
#include <vector>
#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

const int numAttempts = 20;

Synthesizer::Synthesizer(InputSettings * newSettings, microseconds & synthesisTime) {
	auto startTime = high_resolution_clock::now();
	settings = newSettings;
	size = settings->size;
	blockSize = settings->blockSize;
	numLabels = settings->numLabels;
	offset = new int[3];

	for (int dim = 0; dim < 3; dim++) {
		// If we are shifting the block along this dimension, we need to leave room for a boundary
		// cell and so the block is offset from the model.
		// NOTE: It perhaps would have been better not to conditionally shift the cells. This saves
		// some memory, but complicates the code.
		if (size[dim] > blockSize[dim]) {
			offset[dim] = 1;
		} else {
			offset[dim] = 0;
		}
	}
	int* possibilitySize = new int[3];
	for (int dim = 0; dim < 3; dim++) {
		if (blockSize[dim] == size[dim]) {
			possibilitySize[dim] = size[dim];
		} else {
			// Add one cell border to each side of the block if we are modifying in blocks.
			possibilitySize[dim] = blockSize[dim] + 2;
		}
	}

	if (settings->useAc4) {
		propagator = new PropagatorAc4(newSettings, possibilitySize, offset);
	} else {
		propagator = new PropagatorAc3(newSettings, possibilitySize, offset);
	}

	// Create the model with the initial labels.
	model = new int** [size[0]];
	for (int x = 0; x < size[0]; x++) {
		model[x] = new int* [size[1]];
		for (int y = 0; y < size[1]; y++) {
			model[x][y] = new int[size[2]];
		}
	}
	savedBlock = new int** [possibilitySize[0]];
	for (int x = 0; x < possibilitySize[0]; x++) {
		savedBlock[x] = new int* [possibilitySize[1]];
		for (int y = 0; y < possibilitySize[1]; y++) {
			savedBlock[x][y] = new int[possibilitySize[2]];
		}
	}

	auto endTime = high_resolution_clock::now();
	synthesisTime += duration_cast<microseconds>(endTime - startTime);
}

Synthesizer::~Synthesizer() {
	for (int x = 0; x < size[0]; x++) {
		for (int y = 0; y < size[1]; y++) {
			delete[] model[x][y];
		}
		delete[] model[x];
	}
	delete[] model;
	delete propagator;
}

int*** Synthesizer::getModel() {
	return model;
}

int setupStepValues(const int dim, const int step, const int* shifts, const int* maxBlockStart, bool* hasBoundary) {
	int value = step * shifts[dim];
	hasBoundary[2 * dim] = (step > 0);
	if (value >= maxBlockStart[dim]) {
		value = maxBlockStart[dim];
		hasBoundary[2 * dim + 1] = false;
	}
	else {
		hasBoundary[2 * dim + 1] = true;
	}
	return value;
}

enum PrintMode { NONE, TEXT, NEW_LINE};

void printIteration(int blockStart, PrintMode printMode, int indentation, string varName) {
	if (printMode == NEW_LINE) {
		for (int i = 0; i < indentation; i++) {
			cout << "   ";
		}
		cout << endl << varName << " = " << blockStart << endl;
	} else if (printMode == TEXT) {
		if (blockStart == 0) {
			for (int i = 0; i < indentation; i++) {
				cout << "   ";
			}
			cout << varName << " = " << blockStart;
		} else {
			cout << ", " << blockStart;
		}
	}
}

void Synthesizer::synthesize(microseconds& synthesisTime) {
	auto startTime = high_resolution_clock::now();

	// Set the initial labels.
	for (int x = 0; x < size[0]; x++) {
		for (int y = 0; y < size[1]; y++) {
			for (int z = 0; z < size[2]; z++) {
				model[x][y][z] = settings->initialLabels[z];
			}
		}
	}

	// At each step, we shift the block by half the width of the block.
	// Calculate how many steps are needed.
	int shifts[3];
	int numSteps[3];
	int maxBlockStart[3];
	int blockStart[3];
	for (int dim = 0; dim < 3; dim++) {
		shifts[dim] = max(blockSize[dim] / 2, 1);
		numSteps[dim] = (int)ceil((size[dim] - blockSize[dim]) / ((float)shifts[dim])) + 1;
		maxBlockStart[dim] = size[dim] - blockSize[dim];
	}
	// Logic for how to print the iterations.
	PrintMode printMode[3];
	int indentation[3];
	int indent = 0;
	int lastPrint = -1;
	for (int dim = 0; dim < 3; dim++) {
		indentation[dim] = indent;
		if (numSteps[dim] > 1) {
			printMode[dim] = NEW_LINE;
			lastPrint = dim;
			indent++;
		} else {
			printMode[dim] = NONE;
		}
	}
	if (lastPrint >= 0) {
		printMode[lastPrint] = TEXT;
	}

	int blocks = 0;


	bool modifyInBlocks = (lastPrint >= 0);
	// Whether or not we should fill in the boundary values. We do not do 
	// this if they would be outside the model. This is for the boundary
	// in six directions: -X, +X, -Y, +Y, -Z, +Z.
	bool hasBoundary[6];
	for (int xStep = 0; xStep < numSteps[0]; xStep++) {
		blockStart[0] = setupStepValues(0, xStep, shifts, maxBlockStart, hasBoundary);
		printIteration(blockStart[0], printMode[0], indentation[0], "x");
		for (int yStep = 0; yStep < numSteps[1]; yStep++) {
			blockStart[1] = setupStepValues(1, yStep, shifts, maxBlockStart, hasBoundary);
			printIteration(blockStart[1], printMode[1], indentation[1], "y");
			for (int zStep = 0; zStep < numSteps[2]; zStep++) {
				blockStart[2] = setupStepValues(2, zStep, shifts, maxBlockStart, hasBoundary);
				printIteration(blockStart[2], printMode[2], indentation[2], "z");
				// If the model is in 3D we also include a boundary for z-values to force a
				// ground plane to appear.
				if (size[2] > 1) {
					hasBoundary[4] = true;
					hasBoundary[5] = true;
				}
				bool success = false;
				int attempts = 0;
				if (modifyInBlocks) {
					saveBlock(blockStart);
				}
				while (!success && attempts < numAttempts) {
					success = synthesizeBlock(blockStart, hasBoundary);
					attempts++;
					if (!success) {
						if (attempts < numAttempts) {
							if (lastPrint == -1) {
								cout << "  Failed. Retrying..." << endl;
							}
						} else {
							if (modifyInBlocks) {
								restoreBlock(blockStart);
							}
							cout << "  Failed. Max Attempts." << endl;
						}
					}
				}
			}
		}
	}
	if (lastPrint >= 0) {
		cout << endl;
	}
	auto endTime = high_resolution_clock::now();
	synthesisTime += duration_cast<microseconds>(endTime - startTime);
}

// Find the label at a given position in the model.
int Synthesizer::getLabel(int position[3]) {
	return model[position[0]][position[1]][position[2]];
}

// Adds all the labels on the boundary of the blocks in a particular direction.
void Synthesizer::addBoundary(int blockStart[3], int dir) {
	// dim1 and dim2 are the two other dimensions.
	int dim0 = dir / 2;
	int dim1 = -1;
	int dim2 = -1;
	switch (dim0) {
		case 0: dim1 = 1; dim2 = 2; break;
		case 1: dim1 = 0; dim2 = 2; break;
		case 2: dim1 = 0; dim2 = 1; break;
	}

	int blockPos[3];
	int modelPos[3];
	if (dir % 2) {
		blockPos[dim0] = blockSize[dim0] - 1 + offset[dim0];
	} else {
		blockPos[dim0] = 0;
	}
	modelPos[dim0] = blockPos[dim0] + blockStart[dim0] - offset[dim0];
	for (int i = offset[dim1]; i < blockSize[dim1] + offset[dim1]; i++) {
		blockPos[dim1] = i;
		modelPos[dim1] = i + blockStart[dim1] - offset[dim1];
		for (int j = offset[dim2]; j < blockSize[dim2] + offset[dim2]; j++) {
			blockPos[dim2] = j;
			modelPos[dim2] = j + blockStart[dim2] - offset[dim2];
			propagator->setBlockLabel(getLabel(modelPos), blockPos);
		}
	}
}

// Set the labels to create a ground plane.
void Synthesizer::addGround(int blockStart[3]) {
	if (blockSize[1] - offset[1] + blockStart[1] + offset[1] == size[1]) {
		int position[3];
		position[2] = 0;
		for (int x = offset[0]; x < blockSize[0] + offset[0]; x++) {
			position[0] = x;
			for (int y = offset[1]; y < blockSize[1] + offset[1]; y++) {
				position[1] = y;
				if (y == blockSize[1] - 1) {
					propagator->setBlockLabel(settings->ground, position);
				} else if (propagator->isPossible(x, y, 0, settings->ground)) {
					propagator->removeLabel(settings->ground, position);
				}
			}
		}
	}
}

// Remove labels with no support in any particular direction. Those labels
// can only be on the boundary of the model.
void Synthesizer::removeNoSupport(int blockStart[3]) {
	int numDirections = 2 * settings->numDims;
	std::deque<vector<int>> updateQueue;
	for (int i = 0; i < numLabels; i++) {
		for (int dir = 0; dir < numDirections; dir++) {
			if (settings->supportCount[i][dir] == 0) {
				int position[3];
				for (int x = offset[0]; x < blockSize[0] + offset[0]; x++) {
					position[0] = x;
					for (int y = offset[1]; y < blockSize[1] + offset[1]; y++) {
						position[1] = y;
						for (int z = offset[2]; z < blockSize[2] + offset[2]; z++) {
							position[2] = z;
							bool remove = false;
							switch (dir) {
								case 0: remove = x + blockStart[0] - offset[0] != size[0] - 1; break;
								case 1: remove = x + blockStart[0] - offset[0] != 0; break;
								case 2: remove = y + blockStart[1] - offset[1] != size[1] - 1; break;
								case 3: remove = y + blockStart[1] - offset[1] != 0; break;
								case 4: remove = z + blockStart[2] - offset[2] != size[2] - 1; break;
								case 5: remove = z + blockStart[2] - offset[2] != 0; break;
							}
							if (remove && propagator->isPossible(x, y, z, i)) {
								propagator->removeLabel(i, position);
							}
						}
					}
				}
			}
		}
	}
}

void Synthesizer::saveBlock(int blockStart[3]) {
	for (int x = offset[0]; x < blockSize[0] + offset[0]; x++) {
		for (int y = offset[1]; y < blockSize[1] + offset[1]; y++) {
			for (int z = offset[2]; z < blockSize[2] + offset[2]; z++) {
				savedBlock[x][y][z] = 
					model[x + blockStart[0] - offset[0]]
					     [y + blockStart[1] - offset[1]]
				         [z + blockStart[2] - offset[2]];
			}
		}
	}
}

void Synthesizer::restoreBlock(int blockStart[3]) {
	for (int x = offset[0]; x < blockSize[0] + offset[0]; x++) {
		for (int y = offset[1]; y < blockSize[1] + offset[1]; y++) {
			for (int z = offset[2]; z < blockSize[2] + offset[2]; z++) {
				model[x + blockStart[0] - offset[0]]
					 [y + blockStart[1] - offset[1]]
				     [z + blockStart[2] - offset[2]] = savedBlock[x][y][z];
			}
		}
	}
}

// Modifying the labels within a particular block. The block can be as big
// as the whole model.
bool Synthesizer::synthesizeBlock(int blockStart[3], bool hasBoundary[6]) {
	propagator->resetBlock();
	for (int dir = 0; dir < 6; dir++) {
		if (hasBoundary[dir]) {
			addBoundary(blockStart, dir);
		}
	}
	if (settings->ground >= 0) {
		addGround(blockStart);
	}
	if (settings->useAc4) {
		// removeNoSupport is only necessary for AC-4.
		// In AC-3 theses labels are removed during propagation.
		removeNoSupport(blockStart);
	}

	for (int x = offset[0]; x < blockSize[0] + offset[0]; x++) {
		for (int y = offset[1]; y < blockSize[1] + offset[1]; y++) {
			for (int z = offset[2]; z < blockSize[2] + offset[2]; z++) {
				int label = propagator->pickLabel(x, y, z);
				if (label == -1) {
					return false;
				}
				model[x + blockStart[0] - offset[0]]
					 [y + blockStart[1] - offset[1]]
				     [z + blockStart[2] - offset[2]] = label;
			}
		}
	}
	return true;
}

void Synthesizer::printModel() {
	cout << "----------------------------------" << endl;
	cout << "Model" << endl;
	for (int z = 0; z < size[2]; z++) {
		for (int y = 0; y < size[1]; y++) {
			for (int x = 0; x < size[0]; x++) {
				cout << model[x][y][z] << " ";
			}
			cout << endl;
		}
		cout << endl;
	}
	cout << "----------------------------------" << endl;
}