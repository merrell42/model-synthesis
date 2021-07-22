// Copyright (c) 2021 Paul Merrell
#ifndef OUTPUT_GENERATOR
#define OUTPUT_GENERATOR

#include "parseInput/parseInput.h"
#include <string>
#include <chrono>

void generateOutput(
	const InputSettings& settings,
	int*** model,
	const std::string outputPath,
	std::chrono::microseconds& outputTime);

#endif // OUTPUT_GENERATOR