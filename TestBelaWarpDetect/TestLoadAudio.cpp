//
//  TestLoadAudio.cpp
//  TestBelaWarpDetect
//
//  Created by Nathan Perkins on 1/19/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <stdio.h>
#include <vector>

#include "catch.hpp"

#include "LoadAudio.hpp"

#define COMPARE_FLOAT(a, b) (abs((a) - (b)) < 1e-6)
#define COMPARE_FLOAT_THRESH(a, b, threshold) (abs((a) - (b)) < threshold)

TEST_CASE("Testing Load Audio") {
    SECTION("Mono Sin") {
        // uses sin.wav
        // generated in MATLAB:
        // fs = 44100; t = 0:(1/fs):1; audiowrite('sin.wav', sin(800 * 2 * pi * t), fs);
        
        std::vector<float> audio;
        float sample_rate = 0;
        
        // successfully load file?
        REQUIRE(LoadAudio("sin.wav", audio, sample_rate));
        
        // check sample rate
        CHECK(COMPARE_FLOAT(sample_rate, 44100));
        
        // spot check first few
        for (unsigned int i = 0, maxi = 100; i < maxi; ++i) {
            float t = (float)i / sample_rate;
            float v = sin(800.0 * 2.0 * M_PI * t);
            CAPTURE(audio[i]);
            CHECK(COMPARE_FLOAT_THRESH(audio[i], v, 1e-4));
        }
    }
    
    SECTION("Stereo Sin") {
        // uses sin2.wav
        // generated in MATLAB:
        // fs = 44100; t = 0:(1/fs):1); audiowrite('sin2.wav', [sin(800 * 2 * pi * t); sin(1600 * 2 * pi * t)]', fs);
        
        std::vector<float> audio;
        float sample_rate = 0;
        
        // successfully load file?
        REQUIRE(LoadAudio("sin2.wav", audio, sample_rate));
        
        // check sample rate
        CHECK(COMPARE_FLOAT(sample_rate, 44100));
        
        // spot check first few
        for (unsigned int i = 0, maxi = 100; i < maxi; ++i) {
            float t = (float)i / sample_rate;
            float v = sin(800.0 * 2.0 * M_PI * t);
            CAPTURE(audio[i]);
            CHECK(COMPARE_FLOAT_THRESH(audio[i], v, 1e-4));
        }
        
        // successfully load file?
        REQUIRE(LoadAudio("sin2.wav", audio, sample_rate, 1));
        
        // check sample rate
        CHECK(COMPARE_FLOAT(sample_rate, 44100));
        
        // spot check first few
        for (unsigned int i = 0, maxi = 100; i < maxi; ++i) {
            float t = (float)i / sample_rate;
            float v = sin(1600.0 * 2.0 * M_PI * t);
            CAPTURE(audio[i]);
            CHECK(COMPARE_FLOAT_THRESH(audio[i], v, 1e-4));
        }
    }
}
