//
//  CircularShortTimeFourierTransform.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/11/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include "CircularShortTimeFourierTransform.hpp"

#include <iostream>
#include <cmath>

CircularShortTermFourierTransform::CircularShortTermFourierTransform() {
    _initialized = false;
}

CircularShortTermFourierTransform::~CircularShortTermFourierTransform() {
    if (_initialized) {
        
        // platform specific resources
#if defined(__APPLE__)
        vDSP_DFT_DestroySetup(_fft_config);
        
        free(_fft_input.realp);
        free(_fft_input.imagp);
        free(_fft_output.realp);
        free(_fft_output.imagp);
#else
        NE10_FREE(_fft_config);
        
        NE10_FREE(_fft_output);
#endif
        
        // release resources
        free(_window);
        free(_buffer);
        free(_samples_windowed);
    }
}

bool CircularShortTermFourierTransform::Initialize(unsigned int window_length, unsigned int window_stride) {
    // already initialized?
    if (_initialized) {
        return false;
    }
    
    _buffer_size = 4096;
    
    _window_length = (fft_length_t)window_length;
    _window_stride = (fft_stride_t)window_stride;
    
    _fft_size = (fft_length_t)ceil(log2(window_length));
    _fft_length = 1 << _fft_size;
    _fft_length_half = _fft_length / 2;
    
    // allocate buffer
    _buffer = (fft_value_t *)malloc(sizeof(fft_value_t) * _buffer_size);
    _ptr_read = 0;
    _ptr_write = 0;
    
    // allocate other memory
    _window = (fft_value_t *)malloc(sizeof(fft_value_t) * _window_length);
    _samples_windowed = (fft_value_t *)calloc(_fft_length, sizeof(fft_value_t));
    
    // initialize window to 1s
    for (unsigned int i = 0; i < _window_length; ++i) {
        _window[i] = 1.;
    }
    
    // platform specific memory
#if defined(__APPLE__)
    _fft_input.realp = (fft_value_t *)malloc(sizeof(fft_value_t) * _fft_length_half);
    _fft_input.imagp = (fft_value_t *)malloc(sizeof(fft_value_t) * _fft_length_half);
    _fft_output.realp = (fft_value_t *)malloc(sizeof(fft_value_t) * _fft_length_half);
    _fft_output.imagp = (fft_value_t *)malloc(sizeof(fft_value_t) * _fft_length_half);
    
    _fft_config = vDSP_DFT_zrop_CreateSetup(NULL, _fft_length, vDSP_DFT_FORWARD);
#else
    _fft_output = (ne10_fft_cpx_float32_t *)NE10_MALLOC(sizeof(ne10_fft_cpx_float32_t) * _fft_length);
    
    _fft_config = ne10_fft_alloc_r2c_float32(_fft_length);
#endif
    
    // success
    _initialized = true;
    return true;
}

bool CircularShortTermFourierTransform::SetWindow(const std::vector<fft_value_t>& window) {
    // initialized
    if (!_initialized) {
        return false;
    }
    
    // check window length
    if (window.size() != _window_length) {
        return false;
    }
    
    // could use memcpy, but...
    for (int i = 0; i < _window_length; ++i) {
        _window[i] = window[i];
    }
    
    return true;
}

// get length
unsigned int CircularShortTermFourierTransform::GetLengthValues() {
    if (!_initialized) {
        return 0;
    }
    
    if (_ptr_write >= _ptr_read) {
        return _ptr_write - _ptr_read;
    }
    
    return _buffer_size + _ptr_write - _ptr_read;
}

unsigned int CircularShortTermFourierTransform::GetLengthCapacity() {
    if (!_initialized) {
        return 0;
    }
    
    // ptr_read == ptr_write means empty, therefore can not be completely full
    // can store up to buffer_size - 1
    
    // no loop around the end
    if (_ptr_write >= _ptr_read) {
        return _buffer_size - (1 + _ptr_write - _ptr_read);
    }
    
    return _buffer_size - (1 + _buffer_size + _ptr_write - _ptr_read);
}

unsigned int CircularShortTermFourierTransform::GetLengthPower() {
    if (!_initialized) {
        return 0;
    }
    
    return _fft_length_half + 1;
}

// write to the circular buffer
bool CircularShortTermFourierTransform::WriteValues(const std::vector<fft_value_t>& values) {
    // check for sufficient space
    if (values.size() > GetLengthCapacity()) {
        return false;
    }
    
    // TODO: rewrite to use copy
    for (std::vector<fft_value_t>::const_iterator it = values.begin(); it != values.end(); ++it) {
        _buffer[_ptr_write] = *it;
        _ptr_write = (_ptr_write + 1) % _buffer_size;
    }
    
    return true;
}

bool CircularShortTermFourierTransform::WriteValues(const fft_value_t *values, const unsigned int len) {
    // check for sufficient space
    if (len > GetLengthCapacity()) {
        return false;
    }
    
    // TODO: rewrite to use copy
    for (unsigned int i = 0; i < len; ++i) {
        _buffer[_ptr_write] = values[i];
        _ptr_write = (_ptr_write + 1) % _buffer_size;
    }
    
    return true;
}

// read power
bool CircularShortTermFourierTransform::ReadPower(fft_value_t *power) {
    // check for sufficient values
    if (GetLengthValues() < _window_length) {
        return false;
    }
    
    // window samples
    for (unsigned int i = 0; i < _window_length; ++i) {
        _samples_windowed[i] = _buffer[(_ptr_read + i) % _buffer_size] * _window[i];
    }
    
    // advance read pointer
    _ptr_read = (_ptr_read + _window_stride) % _buffer_size;
    
#if defined(__APPLE__)
    // pack samples
    vDSP_ctoz((DSPComplex *)_samples_windowed, 2, &_fft_input, 1, _fft_length_half);
    
    // calculate fft
    vDSP_DFT_Execute(_fft_config, _fft_input.realp, _fft_input.imagp, _fft_output.realp, _fft_output.imagp);
    
    // zero imaginary first term (term 0 and term _fft_length_half are both real, and so get
    // packed into the same term)
    power[_fft_length_half] = abs(_fft_output.imagp[0]);
    _fft_output.imagp[0] = 0.;
    
    // power
    vDSP_zvabs(&_fft_output, 1, power, 1, _fft_length_half);
    
    // normalize
    float c_two = 0.5;
    vDSP_vsmul(power, 1, &c_two, power, 1, _fft_length_half + 1);
#else
    // claculate FFT
    ne10_fft_r2c_1d_float32_neon(_fft_output, _samples_windowed, _fft_config);
    
    for (unsigned int i = 0; i < _fft_length_half + 1; ++i) {
        power[i] = sqrt(pow(_fft_output[i].r, 2.) + pow(_fft_output[i].i, 2.));
    }
#endif
    
    return true;
}
