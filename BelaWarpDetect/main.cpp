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

#include "CircularShortTimeFourierTransform.hpp"

int main(int argc, const char * argv[]) {
    CircularShortTermFourierTransform stft;
    stft.Initialize(256, 224);
    
    // profile
    clock_t begin, end;
    double tm = 0;
    unsigned int iter = 100;
    for (unsigned int i = 0; i < iter; ++i) {
        stft.WriteValues(values);
        
        begin = clock();
        stft.ReadPower(&power[0]);
        end = clock();
        
        tm += (double)(end - begin) / CLOCKS_PER_SEC;
    }
    printf("Avg Time: %.5fms\n", tm * 1000 / (double)iter);
    
    return 0;
}
