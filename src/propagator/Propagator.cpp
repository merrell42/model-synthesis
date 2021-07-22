// Copyright (c) 2021 Paul Merrell
#include "Propagator.h"
#include <vector>
#include <iostream>

using namespace std;

// Pick a random value given the weights. Higher weight means higher probability.
int pickFromWeights(float* weights, int n) {
	float sum = 0;
	vector<float> cumulativeSums;
	for (int i = 0; i < n; i++) {
		sum += weights[i];
		cumulativeSums.push_back(sum);
	}
	if (sum == 0) {
		return -1;
	}
	float randomValue = sum * static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	for (int i = 0; i < n; i++) {
		if (randomValue < cumulativeSums[i]) {
			return i;
		}
	}
	return -1;
}

int Propagator::pickLabel(int x, int y, int z) {
	float* weights = new float[numLabels];
	for (int i = 0; i < numLabels; i++) {
		if (isPossible(x, y, z, i)) {
			weights[i] = settings->weights[i];
		} else {
			weights[i] = 0.0;
		}
	}
	int label = pickFromWeights(weights, numLabels);
	if (label == -1) {
		return -1;
	}
	int position[3];
	position[0] = x;
	position[1] = y;
	position[2] = z;
	bool success = setBlockLabel(label, position);
	if (success) {
		return label;
	} else {
		return -1;
	}
}

void Propagator::printPossible(int x, int y, int z) {
	for (int i = 0; i < numLabels; i++) {
		if (isPossible(x, y, z, i)) {
			cout << i << " ";
		}
	}
	cout << endl;
}