//
//  TestManagedMemory.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/23/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <stdio.h>
#include <stdexcept>

#include "catch.hpp"

#include "ManagedMemory.hpp"

#define COMPARE_FLOAT(a, b) (abs((a) - (b)) < 1e-6)
#define COMPARE_FLOAT_THRESH(a, b, threshold) (abs((a) - (b)) < threshold)

TEST_CASE("Testing Managed Memory") {
    SECTION("Allocate Zeros") {
        size_t size = 20;
        
        ManagedMemory<float> mem = ManagedMemory<float>(size);
        
        REQUIRE(mem.size() == size);
        
        for (size_t i = 0; i < size; ++i) {
            // pointer access
            CHECK(*(mem.ptr() + i) == 0.0);
            
            // subscript access
            CHECK(mem[i] == 0.0);
        }
    }
    
    SECTION("Subscript Access") {
        size_t size = 20;
        
        ManagedMemory<float> mem = ManagedMemory<float>(size);
        
        // write
        for (size_t i = 0; i < size; ++i) {
            mem[i] = (float)i;
        }
        
        
        for (size_t i = 0; i < size; ++i) {
            // pointer access
            CHECK(*(mem.ptr() + i) == (float)i);
            
            // subscript access
            CHECK(mem[i] == (float)i);
        }
    }
    
    SECTION("Copy") {
        size_t size = 20;
        
        ManagedMemory<float> mem = ManagedMemory<float>(size);
        
        // copy before
        ManagedMemory<float> mem_before = mem;
        
        // write
        for (size_t i = 0; i < size; ++i) {
            mem[i] = (float)i;
        }
        
        // copy after
        ManagedMemory<float> mem_after = mem;
        
        // check both
        for (size_t i = 0; i < size; ++i) {
            CHECK(mem[i] == (float)i);
            CHECK(mem_before[i] == 0.0);
            CHECK(mem_after[i] == (float)i);
        }
    }
    
    // assignment
    SECTION("Assign") {
        size_t size = 20;

        ManagedMemory<float> mem = ManagedMemory<float>(size);
        ManagedMemory<float> mem2 = ManagedMemory<float>(size);
        
        // write
        for (size_t i = 0; i < size; ++i) {
            mem[i] = (float)i;
        }

        mem2 = mem;
        
        // overwrite
        for (size_t i = 0; i < size; ++i) {
            mem[i] = 0.0;
        }
        
        // check both
        for (size_t i = 0; i < size; ++i) {
            CHECK(mem[i] == 0.0);
            CHECK(mem2[i] == (float)i);
        }
    }
    
    SECTION("Exceptions") {
        size_t size = 20;
        
        ManagedMemory<float> mem = ManagedMemory<float>(size);
        
        CHECK_NOTHROW(mem[19]);
        CHECK_THROWS_AS(mem[20], std::out_of_range);
        
        ManagedMemory<float> mem2 = ManagedMemory<float>(size + 1);
        
        CHECK_THROWS_AS(mem2 = mem, std::invalid_argument);
    }
}
