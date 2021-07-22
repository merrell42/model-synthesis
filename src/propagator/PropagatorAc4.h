// Copyright (c) 2021 Paul Merrell
#ifndef PROPAGATOR_AC_4
#define PROPAGATOR_AC_4

#include <deque>
#include <vector>
#include "Propagator.h"

using namespace std;

class PropagatorAc4 : public Propagator {
	private:
		InputSettings* settings;
		bool**** possibleLabels;
		int***** support;
		int* possibilitySize;
		int* offset;
		int* size;
		int numLabels;
		int numDirections;

		// Propagate the existing labels.
		void propagate(std::deque<vector<int>>& updateQueue);

	public:
		PropagatorAc4(InputSettings* newSettings, int* newPossibilitySize, int* newOffset);
		~PropagatorAc4();

		// Set a label in the block at the given position.
		bool setBlockLabel(int label, int position[3]);

		// Remove a label from the given position.
		bool removeLabel(int label, int position[3]);

		// Reset the block to include all possible labels.
		void resetBlock();

		// Returns true if the label at this location is possible.
		bool isPossible(int x, int y, int z, int label);
};

#endif // PROPAGATOR_AC_4
