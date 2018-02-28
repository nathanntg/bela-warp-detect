//
//  LoadAudio.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/19/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include "LoadAudio.hpp"

#include <sndfile.h>

bool LoadAudio(std::string file, std::vector<float> &samples, float &sample_rate, unsigned int channel) {
    SNDFILE *sf;
    SF_INFO sf_info;
    sf_info.format = 0;
    
    // open file
    if (!(sf = sf_open(file.c_str(), SFM_READ, &sf_info))) {
        return false;
    }
    
    // unexpected number of channels
    if (channel >= sf_info.channels) {
        return false;
    }
    
    // sample rate
    sample_rate = static_cast<float>(sf_info.samplerate);
    
    // prepare memory
    float *mem = static_cast<float *>(malloc(sizeof(float) * sf_info.channels * sf_info.frames));
    
    // seek start frame
    sf_seek(sf, 0, SEEK_SET);
    
    // read
    sf_count_t read_count = sf_read_float(sf, mem, sf_info.channels * sf_info.frames);
    
    // fail if did not receive expected number of samples
    if (read_count != (sf_info.channels * sf_info.frames)) {
        sf_close(sf);
        free(mem);
        return false;
    }
    
    // zero pad, in case file reading failed (maybe return false?)
//    for (unsigned int i = read_count; i < sf_info.frames * sf_info.channels; ++i) {
//        samples[i] = 0.0;
//    }
    
    // scale
    float scale = 1.0;
    
    // re-scale floating point numbers
//    int subformat = sf_info.format & SF_FORMAT_SUBMASK;
//    if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE) {
//        // fetch maximum
//        double t;
//        sf_command(sf, SFC_CALC_SIGNAL_MAX, &t, sizeof(t));
//
//        // if non-zero, use as scaling factor
//        if (t > 1e-10) {
//            // the example code uses 32700.0 as a scaling factor... WHY?
//            scale = 1.0 / t;
//        }
//    }
    
    // resize samples
    samples.resize(sf_info.frames);
    for (unsigned int i = 0; i < sf_info.frames; ++i) {
        samples[i] = mem[i * sf_info.channels + channel] * scale;
    }
    
    // clean up
    sf_close(sf);
    free(mem);
    
    return true;
}
