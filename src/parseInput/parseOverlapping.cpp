#include <iostream>
#include <fstream>
#include <map>
#include "parseInput.h"
#include "../third_party/lodepng/lodepng.h"

using namespace std;

// Returns true if patch B is matches patch A shifted horizontally one pixel.
bool patchesMatchX(vector<unsigned char> a, vector<unsigned char> b, int N) {
	for (int y = 0; y < N; y++) {
		for (int x = 0; x < N - 1; x++) {
			int xyA = rgb(x + 1, y, N);
			int xyB = rgb(x, y, N);
			for (int k = 0; k < 3; k++) {
				if (a[xyA + k] != b[xyB + k]) {
					return false;
				}
			}
		}
	}
	return true;
}

// Returns true if patch B is matches patch A shifted vertically one pixel.
bool patchesMatchY(vector<unsigned char> a, vector<unsigned char> b, int N) {
	for (int y = 0; y < N - 1; y++) {
		for (int x = 0; x < N; x++) {
			int xyA = rgb(x, y + 1, N);
			int xyB = rgb(x, y, N);
			for (int k = 0; k < 3; k++) {
				if (a[xyA + k] != b[xyB + k]) {
					return false;
				}
			}
		}
	}
	return true;
}

// Reflect a patch horizontally.
vector<unsigned char> reflectPatch(std::vector<unsigned char> patch, int N) {
	vector<unsigned char> newPatch;
	for (int y = 0; y < N; y++) {
		for (int x = 0; x < N; x++) {
			int xy = rgb(N - 1 - x, y, N);
			for (int k = 0; k < 3; k++) {
				newPatch.push_back(patch[xy + k]);
			}
		}
	}
	return newPatch;
}

// Rotate a patch 90 degrees.
vector<unsigned char> rotatePatch(std::vector<unsigned char> patch, int N) {
	vector<unsigned char> newPatch;
	for (int y = 0; y < N; y++) {
		for (int x = 0; x < N; x++) {
			int xy = rgb(N - 1 - y, x, N);
			for (int k = 0; k < 3; k++) {
				newPatch.push_back(patch[xy + k]);
			}
		}
	}
	return newPatch;
}

// Get the patch at the given position.
vector<unsigned char> getPatch(int x0, int y0, int w, int h, int N, std::vector<unsigned char> image) {
	vector<unsigned char> patch;
	for (int dy = 0; dy < N; dy++) {
		int y = (y0 + dy) % h;
		for (int dx = 0; dx < N; dx++) {
			int x = (x0 + dx) % w;
			int xy = rgba(x, y, w);
			for (int k = 0; k < 3; k++) {
				patch.push_back(image[xy + k]);
			}
		}
	}
	return patch;
}

// Parse an <overlapping /> input.
void parseOverlapping(InputSettings& settings) {
	string path = "samples/" + settings.name + ".png";
	std::vector<unsigned char> image;
	unsigned w, h;
	std::vector<unsigned char> buffer;
	lodepng::load_file(buffer, path);
	lodepng::State state;
	unsigned error = lodepng::decode(image, w, h, state, buffer);
	if (error) {
		cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	}

	// Default size is 48 x 48.
	if (settings.size[0] == 0) {
		settings.size[0] = 48;
		settings.size[1] = 48;
	}
	int N = settings.tileWidth;
	// Resize block if needed.
	if (settings.blockSize[0] == 0 || settings.blockSize[0] > settings.size[0]) {
		settings.blockSize[0] = settings.size[0];
		settings.blockSize[1] = settings.size[1];
	}

	// Maps from a N x N RGB patch to the number of times the patch appears.
	map<vector<unsigned char>, int> patches;
	// Indicates if a patch could be the ground patch.
	map<vector<unsigned char>, bool> possiblyGround;
	bool hasGround = (settings.ground > 0);
	// If the input is periodic, wrap the tiles around the input.
	const int w0 = (int)w - (settings.periodicInput ? 0 : N - 1);
	const int h0 = (int)h - (settings.periodicInput ? 0 : N - 1);
	for (int y = 0; y < h0; y++) {
		for (int x = 0; x < w0; x++) {
			vector<unsigned char> patch = getPatch(x, y, w, h, N, image);
			if (hasGround && y == h - 1) {
				// Ground tiles are at the bottom.
				possiblyGround[patch] = true;
			}
			patches[patch]++;
			// Reflect and rotate the patches depending on the symmetry.
			for (int i = 1; i < settings.symmetry; i++) {
				if (i % 2 == 1) {
					patches[reflectPatch(patch, N)]++;
				} else {
					patch = rotatePatch(patch, N);
					patches[patch]++;
				}
			}
		}
	}



	int qIndex = 0;
	for (auto aIt = patches.begin(); aIt != patches.end(); aIt++) {
		std::vector<unsigned char> image;
		image.resize(4 * N * N);
		for (int x = 0; x < N; x++) {
			for (int y = 0; y < N; y++) {
				for (int k = 0; k < 3; k++) {
					image[4 * (x + y * N) + k] = aIt->first[3 * (x + y * N) + k];
				}
				image[4 * (x + y * N) + 3] = 255;
			}
		}
		// Encode the image.
		unsigned error = lodepng::encode("patterns/" + to_string(qIndex) + ".png", image, N, N);
		qIndex++;
	}



	// Save images and weights.
	int aIndex = 0;
	for (auto aIt = patches.begin(); aIt != patches.end(); aIt++) {
		settings.tileImages.push_back(aIt->first);
		settings.weights.push_back((float)aIt->second);
		aIndex++;
	}
	int numLabels = aIndex;
	settings.numLabels = numLabels;

	// Compute the transitions.
	bool*** transition = createTransition(numLabels);
	settings.transition = transition;
	aIndex = 0;
	for (auto aIt = patches.begin(); aIt != patches.end(); aIt++) {
		int bIndex = 0;
		vector<unsigned char> a = aIt->first;
		for (auto bIt = patches.begin(); bIt != patches.end(); bIt++) {
			vector<unsigned char> b = bIt->first;
			transition[0][aIndex][bIndex] = patchesMatchX(a, b, N);
			transition[1][aIndex][bIndex] = patchesMatchY(a, b, N);
			bIndex++;
		}
		aIndex++;
	}

	if (hasGround) {
		int g = 0;
		bool groundFound = false;
		for (auto aIt = patches.begin(); aIt != patches.end(); aIt++) {
			if (possiblyGround[aIt->first] && transition[0][g][g]) {
				settings.ground = g;
				groundFound = true;
				break;
			}
			g++;
		}
		if (!groundFound) {
			cout << "Ground label not found." << endl;
		}
	}
	findInitialLabel(settings);
}
