//
//  main.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/11/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <iostream>
#include <vector>
#include <time.h>

#include "MatchSyllables.hpp"
#include "LoadAudio.hpp"

std::vector<std::vector<float>> result_score;
std::vector<std::vector<int>> result_length;

void cbPrintResult(size_t index, float score, int length) {
    std::cout << "Syllabe " << index << ": " << score << " (" << length << ")\n";
}

void cbAppendResult(std::vector<float> scores, std::vector<int> lengths) {
    result_score.push_back(scores);
    result_length.push_back(lengths);
}

int main(int argc, const char * argv[]) {
    std::vector<float> signal;
    float sampling_rate;
    
    // load audio
    if (!LoadAudio("Matlab/audio.wav", signal, sampling_rate)) {
        std::cerr << "Unable to load audio file.\n";
        return 1;
    }
    
    // create matcher
    MatchSyllables ms(sampling_rate);
    
    // add syllable
    if (ms.AddSyllable("Matlab/template.wav", 2900.0) == -1) {
        std::cerr << "Unable to add syllable.\n";
        return 1;
    }
    
    // set callback
    ms.SetCallbackMatch(cbPrintResult);
    ms.SetCallbackColumn(cbAppendResult);
    
    // initialize
    if (!ms.Initialize()) {
        std::cerr << "Unable to initialize syllable matcher.\n";
        return 1;
    }
    
    // chunks
    size_t chunk_size = 1024;
    std::vector<float>::iterator begin = signal.begin();
    for (size_t i = 0; i < signal.size(); i += chunk_size) {
        std::vector<float> chunk = std::vector<float>(begin + i, begin + (i + chunk_size < signal.size() ? i + chunk_size : signal.size()));
        if (!ms.IngestAudio(chunk)) {
            std::cerr << "Error while ingesting audio.\n";
            return 1;
        }
    }
    
    std::cout << result_score.size() << "\n";
    
    return 0;
}
