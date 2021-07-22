// Copyright (c) 2021 Paul Merrell
#ifndef INPUT_SETTINGS
#define INPUT_SETTINGS

#include "../third_party/xmlParser.h"
#include <vector>
#include <chrono>

using namespace std;

struct InputSettings {
	~InputSettings();

	string name;

	// Whether to use the AC-4 algorithm instead of AC-3.
	bool useAc4 = true;

	// The size of the output that should be generated.
	int size[3];

	// The block size in which to modify the model. This is equal
	// to the output size if we are not modifying in blocks.
	int blockSize[3];

	// The weight of each label. Labels with more weight are more
	// likely to be selected.
	vector<float> weights;

	// The number of possible labels in each cell.
	int numLabels = 0;

	// These labels are used to generate the initial model. These are
	// only needed if we are modifying in blocks.
	int* initialLabels;

	// The transition describes which labels can be next to each other.
	// When transition[direction][labelA][labelB] is true that means labelA
	// can be just below labelB in the specified direction where x = 0, y = 1, z = 2.
	bool*** transition;

	// Which labels does each label support in each direction.
	vector<vector<vector<int>>> supporting;

	// The amount that each label is supported in each direction.
	vector<vector<int>> supportCount;

	// The ending of the file for the tiled model.
	string tiledModelSuffix = "";

	// The image bitmaps.
	vector<vector<unsigned char>> tileImages;

	// The width and height in pixels.
	int tileWidth = 0;
	int tileHeight = 0;

	// If the output is periodic in X and Y.
	bool periodic = false;

	// The number of dimensions.
	int numDims = 3;

	// The type of input.
	string type = "";
	string subset = "";

	/** These only apply to overlapping examples. **/
	// Whether the input is periodic.
	bool periodicInput = true;
	// The ground tile. -1 if one is not present.
	int ground = -1;
	// The type of symmetry.
	int symmetry = 0;
};

// Return the index for an RGB image.
int rgb(int x, int y, int N);

// Return the index for an RGBA image.
int rgba(int x, int y, int N);

// Read a floating point attribute from an XML node.
float parseFloat(const XMLNode& node, const wchar_t* attribute, const float defaultValue);

// Read an integer attribute from an XML node.
int parseInt(const XMLNode& node, const wchar_t* attribute, const int defaultValue);

// Read a Boolean attribute from an XML node.
bool parseBool(const XMLNode& node, const wchar_t* attribute, const bool defaultValue);

// Find an initial label that can tile the plane.
void findInitialLabel(InputSettings& settings);

// Create a transition matrix.
bool*** createTransition(int numLabels);

// Delete a transition matrix.
void deleteTransition(bool*** transition, int numLabels);

#endif // INPUT_SETTINGS