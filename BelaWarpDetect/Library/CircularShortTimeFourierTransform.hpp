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

#include "ManagedMemory.hpp"

#if !defined(BELA_MAJOR_VERSION)
#include "TPCircularBuffer.h"
#endif

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
    CircularShortTermFourierTransform(unsigned int window_length, unsigned int window_stride, unsigned int buffer_length = 4096);
    ~CircularShortTermFourierTransform();
    
    // window
    std::vector<fft_value_t> GetWindow();
    bool SetWindow(const std::vector<fft_value_t>& window);
    void SetWindowHanning();
    void SetWindowHamming();
    
    // get length
    unsigned int GetLengthValues();
    unsigned int GetLengthColumns();
    unsigned int GetLengthCapacity();
    unsigned int GetLengthPower();
    
    // clear all data in the circular buffer
    void Clear();
    
    // conversion helper functions
    unsigned int ConvertSamplesToColumns(unsigned int samples);
    unsigned int ConvertColumnsToSamples(unsigned int columns);
    float ConvertIndexToFrequency(unsigned int index, float sample_rate);
    unsigned int ConvertFrequencyToIndex(float frequency, float sample_rate); // next highest frequency bin
    
    // zero pad to edge (add zeros at the end to the nearest column boundary)
    void ZeroPadToEdge();
    
    // write to the circular buffer
    bool WriteValues(const std::vector<fft_value_t>& values);
    bool WriteValues(const fft_value_t *values, const unsigned int len, const unsigned int stride = 1);
    
    // read power
    bool ReadPower(fft_value_t *power);
    bool ReadPower(std::vector<fft_value_t>& power);
    
private:
    // prevent copying
    CircularShortTermFourierTransform(const CircularShortTermFourierTransform &);
    const CircularShortTermFourierTransform &operator=(const CircularShortTermFourierTransform &);
    
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
    
    fft_length_t _window_length;
    fft_length_t _window_stride;
    
    fft_length_t _fft_size;
    fft_length_t _fft_length;
    fft_length_t _fft_length_half;
    
    ManagedMemory<fft_value_t> _window;
#if defined(BELA_MAJOR_VERSION)
    ManagedMemory<fft_value_t> _buffer; // circular buffer used to store values
    unsigned int _ptr_write = 0; // point to write in sample vector
    unsigned int _ptr_read = 0; // point to read in sample vector
#else
    TPCircularBuffer _buffer; // circular buffer
#endif
    
    ManagedMemory<fft_value_t> _samples_windowed; // store windowed values
};

#endif /* CircularShortTimeFourierTransform_hpp */

