//
//  CircularShortTimeFourierTransform.hpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/11/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#ifndef CircularShortTimeFourierTransform_hpp
#define CircularShortTimeFourierTransform_hpp

#include <stdio.h>
#include <vector>

#if defined(__APPLE__)
#include <Accelerate/Accelerate.h>

typedef vDSP_Length fft_length_t;
typedef vDSP_Stride fft_stride_t;
typedef float fft_value_t;
#else


#endif


class CircularShortTermFourierTransform
{
public:
    CircularShortTermFourierTransform();
    ~CircularShortTermFourierTransform();
    
    bool Initialize(unsigned int window_length, unsigned int window_stride);
    bool SetWindow(const std::vector<fft_value_t>& window);
    
    // get length
    unsigned int GetLengthValues();
    unsigned int GetLengthCapacity();
    unsigned int GetLengthPower();
    
    // write to the circular buffer
    bool WriteValues(const std::vector<fft_value_t>& values);
    bool WriteValues(const fft_value_t *values, const unsigned int len);
    
    // read power
    bool ReadPower(fft_value_t *power);
    
private:
    bool _initialized;
    
    unsigned int _buffer_size;
    
    // platform specific variables
#if defined(__APPLE__)
    vDSP_DFT_Setup _fft_config;
    DSPSplitComplex _fft_input;
    DSPSplitComplex _fft_output;
#else
    
    
#endif
    
    fft_length_t _fft_size;
    fft_length_t _fft_length;
    fft_length_t _fft_length_half;
    fft_length_t _window_length;
    fft_length_t _window_stride;
    
    fft_value_t *_window;
    fft_value_t *_buffer; // circular buffer used to store values
    unsigned int _ptr_write; // point to write in sample vector
    unsigned int _ptr_read; // point to read in sample vector
    
    fft_value_t *_samples_windowed; // store windowed values
};

#endif /* CircularShortTimeFourierTransform_hpp */
