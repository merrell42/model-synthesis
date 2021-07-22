// Copyright (c) 2021 Paul Merrell
#ifndef PARSE_INPUT
#define PARSE_INPUT

#include "../third_party/xmlParser.h"
#include "InputSettings.h"
#include <chrono>

InputSettings* parseInput(const XMLNode& node, std::chrono::microseconds& inputTime);

#endif // PARSE_INPUT