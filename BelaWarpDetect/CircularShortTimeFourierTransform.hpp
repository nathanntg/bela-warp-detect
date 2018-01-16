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
#include <ne10/NE10.h> // NEON FFT library

typedef unsigned int fft_length_t;
typedef int fft_stride_t;
typedef ne10_float32_t fft_value_t;
#endif

/// A circular buffer that produces a spectrogram (calculating a short term fourier transform).
class CircularShortTermFourierTransform
{
public:
    CircularShortTermFourierTransform();
    ~CircularShortTermFourierTransform();
    
    bool Initialize(unsigned int window_length, unsigned int window_stride);
    
    // window
    bool GetWindow(std::vector<fft_value_t>& window);
    bool SetWindow(const std::vector<fft_value_t>& window);
    void SetWindowHanning();
    void SetWindowHamming();
    
    // get length
    unsigned int GetLengthValues();
    unsigned int GetLengthCapacity();
    unsigned int GetLengthPower();
    
    // clear all data in the circular buffer
    void Clear();
    
    // write to the circular buffer
    bool WriteValues(const std::vector<fft_value_t>& values);
    bool WriteValues(const fft_value_t *values, const unsigned int len);
    
    // read power
    bool ReadPower(fft_value_t *power);
    bool ReadPower(std::vector<fft_value_t>& power);
    
private:
    bool _initialized;
    
    unsigned int _buffer_size;
    
    // platform specific variables
#if defined(__APPLE__)
    vDSP_DFT_Setup _fft_config;
    DSPSplitComplex _fft_input;
    DSPSplitComplex _fft_output;
#else
    ne10_fft_r2c_cfg_float32_t _fft_config;
    ne10_fft_cpx_float32_t *_fft_output;
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

