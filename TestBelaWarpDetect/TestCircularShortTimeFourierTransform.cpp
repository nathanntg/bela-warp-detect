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

#define COMPARE_FLOAT(a, b) (abs((a) - (b)) < 1e-6)
#define COMPARE_FLOAT_THRESH(a, b, threshold) (abs((a) - (b)) < threshold)

TEST_CASE("Testing Circular STFT Window") {
    SECTION("Even Size") {
        unsigned int size = 16;
        CircularShortTermFourierTransform stft(size, size);
    
        // default window
        SECTION("Default Window") {
            std::vector<float> window = stft.GetWindow();
            REQUIRE(window.size() == size);
            
            for (auto it = window.begin(); it != window.end(); ++it) {
                CHECK(*it == 1.0);
            }
        }
        
        // hanning window
        SECTION("Hanning Window") {
            stft.SetWindowHanning();
            
            std::vector<float> window = stft.GetWindow();
            REQUIRE(window.size() == size);
            
            float expected[] = {0.0337639, 0.1304955, 0.2771308, 0.4538658, 0.6368315, 0.8013173, 0.9251086, 0.9914865, 0.9914865, 0.9251086, 0.8013173, 0.6368315, 0.4538658, 0.2771308, 0.1304955, 0.0337639};
            for (unsigned int i = 0; i < size; ++i) {
                CHECK(COMPARE_FLOAT(window[i], expected[i]));
            }
        }
        
        // hamming window
        SECTION("Hamming Window") {
            stft.SetWindowHamming();
            
            std::vector<float> window = stft.GetWindow();
            REQUIRE(window.size() == size);
            
            float expected[] = {0.08, 0.1197691, 0.2321999, 0.3978522, 0.5880831, 0.77, 0.9121478, 0.9899479, 0.9899479, 0.9121478, 0.77, 0.5880831, 0.3978522, 0.2321999, 0.1197691, 0.08};
            for (unsigned int i = 0; i < size; ++i) {
                CHECK(abs(window[i] - expected[i]) < 1e-6);
            }
        }
    }
    
    SECTION("Odd Size") {
        unsigned int size = 15;
        CircularShortTermFourierTransform stft(size, size);
        
        // default window
        SECTION("Default Window") {
            std::vector<float> window = stft.GetWindow();
            REQUIRE(window.size() == size);
            
            for (auto it = window.begin(); it != window.end(); ++it) {
                CHECK(*it == 1.0);
            }
        }
        
        // hanning window
        SECTION("Hanning Window") {
            stft.SetWindowHanning();
            
            std::vector<float> window = stft.GetWindow();
            REQUIRE(window.size() == size);
            
            float expected[] = {0.0380602, 0.1464466, 0.3086583, 0.5, 0.6913417, 0.8535534, 0.9619398, 1.0, 0.9619398, 0.8535534, 0.6913417, 0.5, 0.3086583, 0.1464466, 0.0380602};
            for (unsigned int i = 0; i < size; ++i) {
                CHECK(abs(window[i] - expected[i]) < 1e-6);
            }
        }
        
        // hamming window
        SECTION("Hamming Window") {
            stft.SetWindowHamming();
            
            std::vector<float> window = stft.GetWindow();
            REQUIRE(window.size() == size);
            
            float expected[] = {0.08, 0.1255543, 0.2531947, 0.4376404, 0.6423596, 0.8268053, 0.9544457, 1.0, 0.9544457, 0.8268053, 0.6423596, 0.4376404, 0.2531947, 0.1255543, 0.08};
            for (unsigned int i = 0; i < size; ++i) {
                CHECK(abs(window[i] - expected[i]) < 1e-6);
            }
        }
    }
}

TEST_CASE("Testing Circular STFT Buffer") {
    unsigned int buffer_size = 4096;
    unsigned int window_length = 256;
    unsigned int power_length = 129; // 1 + (2 ^ ceil(log2(window_length))) / 2
    
    CircularShortTermFourierTransform stft(window_length, 224, buffer_size);
    
    REQUIRE(stft.GetLengthPower() == 129);
    
    SECTION("Empty Size") {
        // check initial size and capacity
        CHECK(stft.GetLengthValues() == 0);
        CHECK(stft.GetLengthCapacity() == (buffer_size - 1));
    }
    
    SECTION("Column Size") {
        // add values
        std::vector<float> values = std::vector<float>(window_length - 1, 0.0);
        stft.WriteValues(values);
    
        // should have just under one column worth of data
        CHECK(stft.GetLengthValues() == (window_length - 1));
        CHECK(stft.GetLengthCapacity() == ((buffer_size - 1) - (window_length - 1)));
        CHECK(stft.GetLengthColumns() == 0);
        
        // add single 0
        float zero = 0.0;
        stft.WriteValues(&zero, 1);
        
        // should have one column worth of data
        CHECK(stft.GetLengthValues() == window_length);
        CHECK(stft.GetLengthCapacity() == ((buffer_size - 1) - window_length));
        CHECK(stft.GetLengthColumns() == 1);
        
        SECTION("Clear") {
            // clear
            stft.Clear();
        
            // check size and capacity
            CHECK(stft.GetLengthValues() == 0);
            CHECK(stft.GetLengthCapacity() == (buffer_size - 1));
        }
    }
    
    SECTION("Zero Pad") {
        // add values
        std::vector<float> values = std::vector<float>(window_length - 1, 0.0);
        stft.WriteValues(values);
        
        // should have just under one column worth of data
        CHECK(stft.GetLengthValues() == (window_length - 1));
        CHECK(stft.GetLengthCapacity() == ((buffer_size - 1) - (window_length - 1)));
        CHECK(stft.GetLengthColumns() == 0);
        
        // zero pad
        stft.ZeroPadToEdge();
        
        // should have one column worth of data
        CHECK(stft.GetLengthValues() == window_length);
        CHECK(stft.GetLengthCapacity() == ((buffer_size - 1) - window_length));
        CHECK(stft.GetLengthColumns() == 1);
    }
    
    SECTION("Calculate For Ones") {
        // add values
        std::vector<float> values = std::vector<float>(window_length, 1.0);
        stft.WriteValues(values);
        
        // check results
        std::vector<float> power;
        REQUIRE(stft.ReadPower(power));
        REQUIRE(power.size() == power_length);
        CAPTURE(power[0]);
        CHECK(COMPARE_FLOAT_THRESH(power[0], (float)window_length, 1e-4));
        for (unsigned int i = 1; i < power_length; ++i) {
            CHECK(COMPARE_FLOAT_THRESH(power[i], 0.0, 1e-4));
        }
    }
}
