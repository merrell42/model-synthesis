// Copyright (c) 2021 Paul Merrell
#ifndef PROPAGATOR_AC_3
#define PROPAGATOR_AC_3

#include <deque>
#include "Propagator.h"

class PropagatorAc3 : public Propagator {
	private:
		InputSettings* settings;
		bool**** possibleLabels;
		bool*** inQueue;
		int* possibilitySize;
		int* size;
		int* offset;
		int numLabels;

		// Propagate the existing labels in a particular direction.
		void propagate(int x, int y, int z, int dir, std::deque<int*>& updateQueue);

	public:
		PropagatorAc3(InputSettings* newSettings, int* newPossibilitySize, int* newOffset);
		~PropagatorAc3();

		// Set a label in the block at the given position.
		bool setBlockLabel(int label, int position[3]);

		// Remove a label from the given position.
		bool removeLabel(int label, int position[3]);

		// Reset the block to include all possible labels.
		void resetBlock();

		// Returns true if the label at this location is possible.
		bool isPossible(int x, int y, int z, int label);
};

#endif // PROPAGATOR_AC_3
