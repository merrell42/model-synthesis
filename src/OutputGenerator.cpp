// Copyright (c) 2021 Paul Merrell
#include <chrono>
#include <fstream>
#include <iostream>
#include "OutputGenerator.h"
#include "third_party/lodepng/lodepng.h"

using namespace std;
using namespace std::chrono;

void generateSimpleTiled(const InputSettings& settings, int*** model, const string outputPath) {
	const int* size = settings.size;
	int tileWidth = settings.tileWidth;
	int tileHeight = settings.tileHeight;
	int fullWidth = size[0] * tileWidth;
	int fullHeight = size[1] * tileHeight;
	std::vector<unsigned char> image;
	image.resize(4 * fullWidth * fullHeight);
	for (int x = 0; x < size[0]; x++) {
		for (int y = 0; y < size[1]; y++) {
			const vector<unsigned char> tile = settings.tileImages[model[x][y][0]];
			for (int yt = 0; yt < tileHeight; yt++) {
				int imageOffset = x * 4 * tileWidth + 4 * (y * tileHeight + yt) * fullWidth;
				int tileOffset = yt * 4 * tileWidth;
				for (int xt = 0; xt < 4 * tileWidth; xt++) {
					image[imageOffset + xt] = tile[tileOffset + xt];
				}
			}
		}
	}
	// Encode the image.
	unsigned error = lodepng::encode(outputPath, image, fullWidth, fullHeight);

	//if there's an error, display it
	if (error) std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
}

void generateOverlapping(const InputSettings& settings, int*** model, const string outputPath) {
	const int* size = settings.size;
	int w = size[0];
	int h = size[1];
	std::vector<unsigned char> image;
	image.resize(4 * w * h);
	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h; y++) {
			const vector<unsigned char> tile = settings.tileImages[model[x][y][0]];
			for (int k = 0; k < 3; k++) {
				image[4 * (x + y * w) + k] = tile[k];
			}
			image[4 * (x + y * w) + 3] = 255;
		}
	}

	// Encode the image.
	unsigned error = lodepng::encode(outputPath, image, w, h);

	//if there's an error, display it
	if (error) std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
}

void generateTiledModel(const InputSettings& settings, int*** model, const string outputPath) {
	const int* size = settings.size;
	ofstream outfile(outputPath, ios::out);
	outfile << "Model generated using Paul Merrell's model synthesis algorithm.  Do not insert or delete lines from this file." << endl;
	outfile << endl;
	outfile << "x, y, and z extents" << endl;
	outfile << size[0] << " " << size[1] << " " << size[2] << endl << endl;

	for (int z = 0; z < size[2]; z++) {
		for (int x = 0; x < size[0]; x++) {
			for (int y = 0; y < size[1]; y++) {
				int label = model[x][y][z];
				if (label < 10) {
					outfile << " ";
				}
				outfile << label << " ";
			}
			outfile << endl;
		}
		outfile << endl;
	}
	outfile << settings.tiledModelSuffix;
	outfile.close();

	ofstream lastfile("outputs/latest.txt", ios::out);
	lastfile << "This file simply records the name of the file of the most recently generated model which is:" << endl;
	lastfile << settings.name << endl;
	lastfile.close();
}

void generateOutput(
	const InputSettings& settings,
	int*** model,
	const std::string outputPath,
	microseconds& outputTime) {
	auto startTime = high_resolution_clock::now();
	if (settings.type == "simpletiled") {
		generateSimpleTiled(settings, model, outputPath);
	} else if (settings.type == "overlapping") {
		generateOverlapping(settings, model, outputPath);
	} else if (settings.type == "tiledmodel") {
		generateTiledModel(settings, model, outputPath);
	}
	auto endTime = high_resolution_clock::now();
	outputTime += duration_cast<microseconds>(endTime - startTime);
}
