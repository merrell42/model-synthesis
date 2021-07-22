// Copyright (c) 2021 Paul Merrell
#include <iostream>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <string>
#include <vector>
#include "parseInput.h"
#include "../third_party/lodepng/lodepng.h"

using namespace std;
using namespace std::chrono;

// Find the index of name within names.
int getIndex(vector<string> names, string name) {
	auto it = find(names.begin(), names.end(), name);
	if (it != names.end()) {
		return it - names.begin();
	}
	else {
		return -1;
	}
}

// Remove the suffix " 0" if it is present.
string remove0(string x) {
	if (x[x.size() - 1] == '0' && x[x.size() - 2] == ' ') {
		x.pop_back();
		x.pop_back();
	}
	return x;
}

inline bool fileExists(const std::string& path) {
	ifstream file(path.c_str());
	return file.good();
}

// Try several different options for the path:
//   1. It could be the original name.
//   2. The name without the number.
//   3. The name with an extra 0.
string findTilePath(string path0, string suffix) {
	if (fileExists(path0 + suffix)) {
		return path0 + suffix;
	}
	string path = path0;
	path.pop_back();
	path.pop_back();
	if (fileExists(path + suffix)) {
		return path + suffix;
	}
	if (fileExists(path0 + " 0" + suffix)) {
		return path0 + " 0" + suffix;
	}
	cout << "ERROR: Image file " << path0 << " does not exist." << endl;
	return "";
}

// Get the version number of the tile from the path.
int numberFromPath(string path) {
	if (path[path.length() - 6] == ' ') {
		return (int)(path[path.length() - 5] - '0');
	} else {
		return 0;
	}
}

// Reads in an image file and reflects and rotates it as needed.
void getTile(string path, int versionNum, InputSettings& settings) {
	std::vector<unsigned char> image;
	unsigned w, h;
	std::vector<unsigned char> buffer;
	lodepng::load_file(buffer, path);
	lodepng::State state;
	unsigned error = lodepng::decode(image, w, h, state, buffer);
	if (error) {
		cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	}
	int version = versionNum - numberFromPath(path);
	std::vector<unsigned char> transformedImage;
	if (version > 0) {
		int w = settings.tileWidth;
		int h = settings.tileHeight;
		transformedImage.resize(4 * w * h);

		// (x0, y0) in the transformed image corresponds to (0, 0) in the original.
		// The u direction corresponds to +x in the original.
		// The v direction corresponds to +y in the original.
		int x0, y0, ux, uy, vx, vy;
		switch (version) {
			case 1:
				x0 = 0; y0 = h - 1;
				ux = 0; uy = -1;
				vx = 1; vy =  0;
				break;
			case 2:
				x0 = w - 1; y0 = h - 1;
				ux = -1; uy = 0;
				vx =  0; vy = -1;
				break;
			case 3:
				x0 = w - 1; y0 = 0;
				ux =  0; uy = 1;
				vx = -1; vy = 0;
				break;
			case 4:
				x0 = w - 1; y0 = 0;
				ux = -1; uy = 0;
				vx = 0; vy = 1;
				break;
			case 5:
				x0 = 0; y0 = 0;
				ux = 0; uy = 1;
				vx = 1; vy = 0;
				break;
			case 6:
				x0 = 0; y0 = h - 1;
				ux = 1; uy = 0;
				vx = 0; vy = -1;
				break;
			case 7:
				x0 = w - 1; y0 = h - 1;
				ux = 0; uy = -1;
				vx = -1; vy = 0;
				break;
			default:
				cout << "Invalid version" << endl;
		}
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				int xr = x0 + ux * x + vx * y;
				int yr = y0 + uy * x + vy * y;
				int xy = rgba(x, y, w);
				int xyr = rgba(xr, yr, w);
				for (int k = 0; k < 4; k++) {
					transformedImage[xyr + k] = image[xy + k];
				}
			}
		}
	} else {
		transformedImage = image;
	}
	settings.tileImages.push_back(transformedImage);
	if (settings.tileHeight == 0) {
		settings.tileWidth = w;
		settings.tileHeight = h;
	} else {
		if (settings.tileHeight != h || settings.tileWidth != w) {
			cout << "ERROR: The tiles do not all have the same size." << endl;
		}
	}
}

// Parse a <simpletiled /> input.
void parseSimpleTiled(InputSettings& settings) {
	// Read in the data.xml file.
	string path = "samples/" + settings.name + "/data.xml";
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wPath = converter.from_bytes(path);
	XMLNode xDataNode = XMLNode::openFileHelper(wPath.c_str(), L"set");
	XMLNode xTilesNode = xDataNode.getChildNode(L"tiles");
	XMLNode xNeighborsNode = xDataNode.getChildNode(L"neighbors");

	// Read in the subset node, if it applies.
	vector<string> subset;
	if (settings.subset != "") {
		wstring wSubset = wstring(settings.subset.begin(), settings.subset.end());
		const XMLNode subsetsNode = xDataNode.getChildNode(L"subsets");
		const XMLNode subsetNode = subsetsNode.getChildNodeWithAttribute(L"subset", L"name", wSubset.c_str());

		int numTiles = subsetNode.nChildNode(L"tile");
		for (int i = 0; i < numTiles; i++) {
			XMLNode tileNode = subsetNode.getChildNode(L"tile", i);
			subset.push_back(tileNode.getAttributeStr(L"name"));
		}
	}

	// Read in each tile. Each tile has a symmetry which tells us how many
	// versions of each tile exist and how rotation and reflection work.
	vector<string> names;
	vector<int> versionNums;
	vector<int> rotation;
	vector<int> reflection;
	int numTiles = xTilesNode.nChildNode(L"tile");
	for (int i = 0; i < numTiles; i++) {
		XMLNode tileNode = xTilesNode.getChildNode(L"tile", i);
		float weight = parseFloat(tileNode, L"weight", 1.0);
		string symmetry = tileNode.getAttributeStr(L"symmetry");
		string name = tileNode.getAttributeStr(L"name");
		if (subset.size() > 0 && getIndex(subset, name) == -1) {
			continue;
		}

		int versions;
		int n = names.size();
		if (symmetry == "X") {
			versions = 1;
			for (int j = 0; j < versions; j++) {
				string vName = name + ((j == 0) ? "" : " " + to_string(j));
				names.push_back(vName);
				settings.weights.push_back(weight);
				versionNums.push_back(j);
				rotation.push_back(n);
				reflection.push_back(n);
			}
		} else if (symmetry == "I") {
			versions = 2;
			for (int j = 0; j < versions; j++) {
				string vName = name + ((j == 0) ? "" : " " + to_string(j));
				names.push_back(vName);
				settings.weights.push_back(weight);
				versionNums.push_back(j);
				rotation.push_back(n + 1 - j);
				reflection.push_back(n + j);
			}
		} else if (symmetry == "\\") {
			versions = 2;
			for (int j = 0; j < versions; j++) {
				string vName = name + ((j == 0) ? "" : " " + to_string(j));
				names.push_back(vName);
				settings.weights.push_back(weight);
				versionNums.push_back(j);
				rotation.push_back(n + 1 - j);
				reflection.push_back(n + 1 - j);
			}
		} else if (symmetry == "L") {
			versions = 4;
			for (int j = 0; j < versions; j++) {
				string vName = name + ((j == 0) ? "" : " " + to_string(j));
				names.push_back(vName);
				settings.weights.push_back(weight);
				versionNums.push_back(j);
				rotation.push_back(n + (j + 1) % 4);
				reflection.push_back(n + (j ^ 1));
			}
		}
		else if (symmetry == "T") {
			versions = 4;
			for (int j = 0; j < versions; j++) {
				string vName = name + ((j == 0) ? "" : " " + to_string(j));
				names.push_back(vName);
				settings.weights.push_back(weight);
				versionNums.push_back(j);
				rotation.push_back(n + (j + 1) % 4);
				reflection.push_back(n + (j % 2 == 0 ? j : 4 - j));
			}
		}
		else if (symmetry == "F") {
			versions = 8;
			for (int j = 0; j < versions; j++) {
				string vName = name + ((j == 0) ? "" : " " + to_string(j));
				names.push_back(vName);
				settings.weights.push_back(weight);
				versionNums.push_back(j);
				rotation.push_back(n + (j + 1) % 4 + (j & 4));
				if (j == 0) {
					reflection.push_back(n + 4);
				} else if (j == 4) {
					reflection.push_back(n);
				} else {
					reflection.push_back(n + (8 - j));
				}
			}
		}
		else {
			cout << "Symmetry " + symmetry + " not supported." << endl;
		}
	}

	// The transition describes which labels can be next to each other.
	int numLabels = names.size();
	bool*** transition = createTransition(numLabels);
	settings.transition = transition;

	vector<int> r = rotation;
	vector<int> f = reflection;
	int numNeighbors = xNeighborsNode.nChildNode(L"neighbor");
	for (int i = 0; i < numNeighbors; i++) {
		 XMLNode neighborNode = xNeighborsNode.getChildNode(L"neighbor", i);
		 string left = remove0(neighborNode.getAttributeStr(L"left"));
		 string right = remove0(neighborNode.getAttributeStr(L"right"));
		 int a = getIndex(names, left);
		 int b = getIndex(names, right);

		 // One of the labels may be missing if we are using a subset.
		 // In that case, ignore this pair of neighbors.
		 if (a == -1 || b == -1) {
			 continue;
		 }

		 // Rotate the neighbors 90 degrees.
		 transition[0][a][b] = true;
		 transition[1][r[b]][r[a]] = true;
		 transition[0][r[r[b]]][r[r[a]]] = true;
		 transition[1][r[r[r[a]]]][r[r[r[b]]]] = true;

		 // Reflect and rotate.
		 transition[0][f[b]][f[a]] = true;
		 transition[1][f[r[b]]][f[r[a]]] = true;
		 transition[0][f[r[r[a]]]][f[r[r[b]]]] = true;
		 transition[1][f[r[r[r[a]]]]][f[r[r[r[b]]]]] = true;
	}

	settings.numLabels = numLabels;
	findInitialLabel(settings);

	for (int i = 0; i < (int)names.size(); i++) {
		const string tilePath = findTilePath("samples/" + settings.name + "/" + names[i], ".png");
		getTile(tilePath, versionNums[i], settings);
	}
}
