//
//  TestCircularShortTimeFourierTransform.cpp
//  TestBelaWarpDetect
//
//  Created by Nathan Perkins on 1/19/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <stdio.h>

#include "catch.hpp"

#include "CircularShortTimeFourierTransform.hpp"

TEST_CASE("Testing Circular STFT Window") {
    CircularShortTermFourierTransform stft;
    std::vector<float> window;
    
    SECTION("Even Size") {
        unsigned int size = 16;
        stft.Initialize(size, size);
    
        // default window
        SECTION("Default Window") {
            REQUIRE(stft.GetWindow(window));
            REQUIRE(window.size() == size);
            
            for (auto it = window.begin(); it != window.end(); ++it) {
                CHECK(*it == 1.0);
            }
        }
        
        // hanning window
        SECTION("Hanning Window") {
            stft.SetWindowHanning();
            REQUIRE(stft.GetWindow(window));
            REQUIRE(window.size() == size);
            
            float expected[] = {0.0337639, 0.1304955, 0.2771308, 0.4538658, 0.6368315, 0.8013173, 0.9251086, 0.9914865, 0.9914865, 0.9251086, 0.8013173, 0.6368315, 0.4538658, 0.2771308, 0.1304955, 0.0337639};
            for (unsigned int i = 0; i < size; ++i) {
                CHECK(abs(window[i] - expected[i]) < 1e-6);
            }
        }
        
        // hamming window
        SECTION("Hamming Window") {
            stft.SetWindowHamming();
            REQUIRE(stft.GetWindow(window));
            REQUIRE(window.size() == size);
            
            float expected[] = {0.08, 0.1197691, 0.2321999, 0.3978522, 0.5880831, 0.77, 0.9121478, 0.9899479, 0.9899479, 0.9121478, 0.77, 0.5880831, 0.3978522, 0.2321999, 0.1197691, 0.08};
            for (unsigned int i = 0; i < size; ++i) {
                CHECK(abs(window[i] - expected[i]) < 1e-6);
            }
        }
    }
    
    SECTION("Odd Size") {
        unsigned int size = 15;
        stft.Initialize(size, size);
        
        // default window
        SECTION("Default Window") {
            REQUIRE(stft.GetWindow(window));
            REQUIRE(window.size() == size);
            
            for (auto it = window.begin(); it != window.end(); ++it) {
                CHECK(*it == 1.0);
            }
        }
        
        // hanning window
        SECTION("Hanning Window") {
            stft.SetWindowHanning();
            REQUIRE(stft.GetWindow(window));
            REQUIRE(window.size() == size);
            
            float expected[] = {0.0380602, 0.1464466, 0.3086583, 0.5, 0.6913417, 0.8535534, 0.9619398, 1.0, 0.9619398, 0.8535534, 0.6913417, 0.5, 0.3086583, 0.1464466, 0.0380602};
            for (unsigned int i = 0; i < size; ++i) {
                CHECK(abs(window[i] - expected[i]) < 1e-6);
            }
        }
        
        // hamming window
        SECTION("Hamming Window") {
            stft.SetWindowHamming();
            REQUIRE(stft.GetWindow(window));
            REQUIRE(window.size() == size);
            
            float expected[] = {0.08, 0.1255543, 0.2531947, 0.4376404, 0.6423596, 0.8268053, 0.9544457, 1.0, 0.9544457, 0.8268053, 0.6423596, 0.4376404, 0.2531947, 0.1255543, 0.08};
            for (unsigned int i = 0; i < size; ++i) {
                CHECK(abs(window[i] - expected[i]) < 1e-6);
            }
        }
    }
}

