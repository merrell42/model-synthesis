// Copyright (c) 2021 Paul Merrell
#include "../third_party/xmlParser.h"
#include "InputSettings.h"
#include <sstream>
#include <iostream>

using namespace std;

InputSettings::~InputSettings() {
	deleteTransition(transition, numLabels);
}

// Read a floating point attribute from an XML node.
float parseFloat(const XMLNode& node, const wchar_t* attribute, const float defaultValue) {
	float result;
	string attributeStr = node.getAttributeStr(attribute);
	if (attributeStr.size() == 0) {
		return defaultValue;
	}
	else {
		stringstream(attributeStr) >> result;
	}
	return result;
}

// Read an integer attribute from an XML node.
int parseInt(const XMLNode& node, const wchar_t* attribute, const int defaultValue) {
	int result;
	string attributeStr = node.getAttributeStr(attribute);
	if (attributeStr.size() == 0) {
		return defaultValue;
	}
	else {
		stringstream(attributeStr) >> result;
	}
	return result;
}

// Read a Boolean attribute from an XML node.
bool parseBool(const XMLNode& node, const wchar_t* attribute, const bool defaultValue) {
	string attributeStr = node.getAttributeStr(attribute);
	if (attributeStr.size() == 0) {
		return defaultValue;
	}
	else {
		if (attributeStr == "True") {
			return true;
		}
		else if (attributeStr == "False") {
			return false;
		}
		else {
			return defaultValue;
		}
	}
}

// Find an initial label that can tile the plane.
void findInitialLabel(InputSettings& settings) {
	settings.initialLabels = new int[1];
	settings.initialLabels[0] = 0;
	bool groundFound = false;
	bool*** transition = settings.transition;
	for (int i = 0; i < settings.numLabels; i++) {
		if (transition[0][i][i] && transition[1][i][i]) {
			settings.initialLabels[0] = i;
			groundFound = true;
			break;
		}
	}
	bool modifyInBlocks = (settings.blockSize[0] < settings.size[0] || settings.blockSize[1] < settings.size[1]);
	if (modifyInBlocks && !groundFound) {
		cout << "The example model has no tileable ground plane. The new model can not be modified in blocks.\n" << endl;
	}
}

// Return the index for an RGB image.
int rgb(int x, int y, int N) {
	return 3 * (x + y * N);
}

// Return the index for an RGBA image.
int rgba(int x, int y, int N) {
	return 4 * (x + y * N);
}

bool*** createTransition(int numLabels) {
	bool*** transition = new bool** [3];
	for (int dir = 0; dir < 3; dir++) {
		transition[dir] = new bool* [numLabels];
		for (int i = 0; i < numLabels; i++) {
			transition[dir][i] = new bool[numLabels];
			for (int j = 0; j < numLabels; j++) {
				transition[dir][i][j] = false;
			}
		}
	}
	return transition;
}

void deleteTransition(bool*** transition, int numLabels) {
	for (int dir = 0; dir < 3; dir++) {
		for (int i = 0; i < numLabels; i++) {
			delete[] transition[dir][i];
		}
		delete[] transition[dir];
	}
	delete[] transition;
}