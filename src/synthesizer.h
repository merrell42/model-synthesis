// Copyright (c) 2021 Paul Merrell
#ifndef SYNTHESIZER
#define SYNTHESIZER

#include "parseInput/parseInput.h"
#include "propagator/Propagator.h"
#include <deque>
#include <vector>
#include <chrono>

class Synthesizer {
	private:
		int*** model;
		int*** savedBlock;
		InputSettings* settings;
		int* size;
		int* blockSize;
		int* offset;
		int numLabels;
		int numDirections;

		// Propagates the set of possible labels.
		Propagator* propagator;

		// Synthesize a block of the model at the offset position and 
		// add the boundary. hasBoundary is whether or not we should fill in the
		// boundary values in the -X, +X, -Y, +Y, -Z, +Z directions.
		bool synthesizeBlock(int blockStart[3], bool hasBoundary[6]);

		// Adds all the labels on the boundary of the blocks in a particular direction.
		void addBoundary(int blockStart[3], int dir);

		// Set the labels to create a ground plane.
		void addGround(int blockStart[3]);

		// Remove labels with no support.
		void removeNoSupport(int blockStart[3]);

		// Return the label at a given position in the model.
		int getLabel(int* position);

		// Save the block we are modifying before we do.
		void saveBlock(int blockStart[3]);

		// Restore the block to its previous state.
		void restoreBlock(int blockStart[3]);

		// This is just for debugging.
		void printModel();

	public:
		Synthesizer(InputSettings* newSettings, std::chrono::microseconds& synthesisTime);
		~Synthesizer();

		// Synthesize a model from the settings.
		void synthesize(std::chrono::microseconds& synthesisTime);

		int*** getModel();
};


#endif
