// Copyright (c) 2021 Paul Merrell
#ifndef PROPAGATOR
#define PROPAGATOR
#include "../parseInput/InputSettings.h"

class Propagator {
	int numLabels;
	InputSettings* settings;

	public:
		Propagator(InputSettings* newSettings) {
			settings = newSettings;
			numLabels = newSettings->numLabels;
		}

		// Set a label in the block at the given position.
		virtual bool setBlockLabel(int label, int position[3]) = 0;

		// Remove a label from the given position.
		virtual bool removeLabel(int label, int position[3]) = 0;

		// Reset the block to include all possible labels.
		virtual void resetBlock() = 0;

		// Returns true if the label at this location is possible.
		virtual bool isPossible(int x, int y, int z, int label) = 0;

		// Pick from one of the possible labels at the (x, y, z). Return -1 if there
		// are none left to choose.
		int pickLabel(int x, int y, int z);

		// Just for debugging.
		void printPossible(int x, int y, int z);
};

#endif // PROPAGATOR
