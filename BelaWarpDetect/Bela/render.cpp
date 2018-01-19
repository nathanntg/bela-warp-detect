//
//  render.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/12/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <Bela.h>

#include "CircularShortTimeFourierTransform.hpp"

bool setup(BelaContext *context, void *userData)
{
    rt_printf("Setup.\n");
    
    CircularShortTermFourierTransform stft;
    stft.Initialize(256, 224);
    
    // build values
    std::vector<float> values;
    for (unsigned int i = 0; i < 256; ++i) {
        values.push_back((float)i / 10.);
    }
    
    rt_printf("Capacity: %d\n", stft.GetLengthCapacity());
    rt_printf("Length: %d\n", stft.GetLengthValues());
    
    // write values
    stft.WriteValues(values);
    
    rt_printf("Capacity: %d\n", stft.GetLengthCapacity());
    rt_printf("Length: %d\n", stft.GetLengthValues());
    
    // read power
    float power[129];
    if (stft.ReadPower(&power[0])) {
        rt_printf("Success!\n");
        
        for (unsigned int i = 0; i < 129; ++i) {
            rt_printf("%d: %f\n", i, power[i]);
        }
    }
    
    return true;
}

void render(BelaContext *context, void *userData)
{
    
}

void cleanup(BelaContext *context, void *userData)
{
    
}
