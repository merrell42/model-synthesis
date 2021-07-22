// Copyright (c) 2021 Paul Merrell
#include <iostream>
#include "src/third_party/xmlParser.h"
#include "src/parseInput/parseInput.h"
#include "src/OutputGenerator.h"
#include "src/Synthesizer.h"
#include <chrono>
#include <vector>
#include <map>

using namespace std;
using namespace std::chrono;

int main() {
    XMLNode xMainNode = XMLNode::openFileHelper(L"samples.xml", L"samples");
    int numSamples = xMainNode.nChildNode();
    int numIterations = 2;

    microseconds inputTime{0}, synthesisTime{0}, outputTime{0};
    for (int i = 0; i < numSamples; i++) {
        InputSettings* settings = parseInput(xMainNode.getChildNode(i), inputTime);
        Synthesizer synthesizer(settings, synthesisTime);
        for (int iteration = 0; iteration < numIterations; iteration++) {
            cout << settings->name << " " << iteration << endl;
            synthesizer.synthesize(synthesisTime);

            string outputPath;
            if (settings->type == "simpletiled" || settings->type == "overlapping") {
                string extra = (i + 1) < 10 ? "0" : "";
                outputPath = "outputs/" + extra + to_string(i + 1) + " " + settings->name + " " + settings->subset + " " + to_string(iteration) + ".png";
            } else {
                outputPath = "outputs/" + to_string(i + 1) + " " + to_string(iteration) + " " + settings->name;
            }
            generateOutput(*settings, synthesizer.getModel(), outputPath, outputTime);
        }
        delete settings;
    }

    // Report computation times.
    double inputTimeMs = inputTime.count() / 1000.0;
    double synthesisTimeMs = synthesisTime.count() / 1000.0;
    double outputTimeMs = outputTime.count() / 1000.0;
    double totalTimeMs = inputTimeMs + synthesisTimeMs + outputTimeMs;
    cout << "Input: " << inputTimeMs << " ms" << endl;
    cout << "Synthesize: " << synthesisTimeMs << " ms" << endl;
    cout << "Output: " << outputTimeMs << " ms" << endl;
    cout << "Total: " << totalTimeMs << " ms" << endl << endl;

    const int successes = numSamples * numIterations;
    cout << "Per Success" << endl;
    cout << "Synthesize: " << synthesisTimeMs / successes << " ms" << endl;
    cout << "Output: " << outputTimeMs / successes << " ms" << endl;
    cout << "Total: " << totalTimeMs / successes << " ms" << endl;

    return 0;
}
