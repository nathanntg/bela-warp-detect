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

CircularShortTermFourierTransform::CircularShortTermFourierTransform(unsigned int window_length, unsigned int window_stride, unsigned int buffer_size) :
_buffer_size(buffer_size),
_window_length(window_length),
_window_stride(window_stride),
_fft_size(static_cast<fft_length_t>(ceil(log2(window_length)))),
_fft_length(1 << _fft_size),
_fft_length_half(_fft_length / 2),
_window(_window_length),
_buffer(buffer_size),
_samples_windowed(_fft_length) {
    // initialize window to 1s
    for (unsigned int i = 0; i < _window_length; ++i) {
        _window[i] = 1.;
    }
    
    // platform specific memory
#if defined(__APPLE__)
    _fft_input.realp = new fft_value_t[_fft_length_half];
    _fft_input.imagp = new fft_value_t[_fft_length_half];
    _fft_output.realp = new fft_value_t[_fft_length_half];
    _fft_output.imagp = new fft_value_t[_fft_length_half];
    
    _fft_config = vDSP_DFT_zrop_CreateSetup(NULL, _fft_length, vDSP_DFT_FORWARD);
#else
    _fft_output = (ne10_fft_cpx_float32_t *)NE10_MALLOC(sizeof(ne10_fft_cpx_float32_t) * _fft_length);
    
    _fft_config = ne10_fft_alloc_r2c_float32(_fft_length);
#endif
}

CircularShortTermFourierTransform::~CircularShortTermFourierTransform() {
    // platform specific resources
#if defined(__APPLE__)
    vDSP_DFT_DestroySetup(_fft_config);
    
    delete[] _fft_input.realp;
    delete[] _fft_input.imagp;
    delete[] _fft_output.realp;
    delete[] _fft_output.imagp;
#else
    NE10_FREE(_fft_config);
    
    NE10_FREE(_fft_output);
#endif
}

std::vector<fft_value_t> CircularShortTermFourierTransform::GetWindow() {
    std::vector<fft_value_t> ret = std::vector<fft_value_t>(_window_length);
    memcpy(&ret[0], _window.ptr(), sizeof(fft_value_t) * _window_length);
    return ret;
}

bool CircularShortTermFourierTransform::SetWindow(const std::vector<fft_value_t>& window) {
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

void CircularShortTermFourierTransform::SetWindowHanning() {
    for (fft_length_t i = 0, j = _window_length - 1; i <= j; ++i, --j) {
        float v = 0.5 * (1.0 - cos(2.0 * M_PI * static_cast<float>(i + 1) / (static_cast<float>(_window_length + 1))));
        _window[i] = v;
        _window[j] = v;
    }
}

void CircularShortTermFourierTransform::SetWindowHamming() {
    for (fft_length_t i = 0, j = _window_length - 1; i <= j; ++i, --j) {
        float v = 0.54 - 0.46 * cos(2.0 * M_PI * static_cast<float>(i) / static_cast<float>(_window_length - 1));
        _window[i] = v;
        _window[j] = v;
    }
}

// get length
unsigned int CircularShortTermFourierTransform::GetLengthValues() {
    if (_ptr_write >= _ptr_read) {
        return _ptr_write - _ptr_read;
    }
    
    return _buffer_size + _ptr_write - _ptr_read;
}

// get length in terms of number of columns (convenience)
unsigned int CircularShortTermFourierTransform::GetLengthColumns() {
    return ConvertSamplesToColumns(GetLengthValues());
}

unsigned int CircularShortTermFourierTransform::GetLengthCapacity() {
    // ptr_read == ptr_write means empty, therefore can not be completely full
    // can store up to buffer_size - 1
    
    // no loop around the end
    if (_ptr_write >= _ptr_read) {
        return _buffer_size - (1 + _ptr_write - _ptr_read);
    }
    
    return _buffer_size - (1 + _buffer_size + _ptr_write - _ptr_read);
}

unsigned int CircularShortTermFourierTransform::GetLengthPower() {
    return _fft_length_half + 1;
}

void CircularShortTermFourierTransform::Clear() {
    _ptr_read = 0;
    _ptr_write = 0;
}

unsigned int CircularShortTermFourierTransform::ConvertSamplesToColumns(unsigned int samples) {
    if (samples < _window_length) {
        return 0;
    }
    
    return 1 + (samples - _window_length) / _window_stride;
}

unsigned int CircularShortTermFourierTransform::ConvertColumnsToSamples(unsigned int columns) {
    if (columns == 0) {
        return 0;
    }
    
    return _window_length + (_window_stride * (columns - 1));
}

float CircularShortTermFourierTransform::ConvertIndexToFrequency(unsigned int index, float sample_rate) {
    return static_cast<float>(index) * sample_rate / static_cast<float>(_fft_length);
}

// next higher frequency bin
unsigned int CircularShortTermFourierTransform::ConvertFrequencyToIndex(float frequency, float sample_rate) {
    return static_cast<unsigned int>(ceil(static_cast<float>(_fft_length) * frequency / sample_rate));
}

void CircularShortTermFourierTransform::ZeroPadToEdge() {
    unsigned int samples = GetLengthValues();
    unsigned int used_samples = ConvertColumnsToSamples(ConvertSamplesToColumns(samples));
    
    // not all samples used? should zero pad
    if (used_samples < samples) {
        unsigned int desired_samples = ConvertColumnsToSamples(ConvertSamplesToColumns(samples) + 1);
        unsigned int diff = desired_samples - samples;
        
        std::vector<fft_value_t> zeros(diff, 0.0);
        WriteValues(zeros);
    }
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

bool CircularShortTermFourierTransform::WriteValues(const fft_value_t *values, const unsigned int len, const unsigned int stride) {
    // check for sufficient space
    if (len > GetLengthCapacity()) {
        return false;
    }
    
    // TODO: rewrite to use copy
    for (unsigned int i = 0, maxi = len * stride; i < maxi; i += stride) {
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
    vDSP_ctoz(reinterpret_cast<DSPComplex *>(_samples_windowed.ptr()), 2, &_fft_input, 1, _fft_length_half);
    
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
    ne10_fft_r2c_1d_float32_neon(_fft_output, _samples_windowed.ptr(), _fft_config);
    
    for (unsigned int i = 0; i < _fft_length_half + 1; ++i) {
        power[i] = sqrt(pow(_fft_output[i].r, 2.) + pow(_fft_output[i].i, 2.));
    }
#endif
    
    return true;
}

bool CircularShortTermFourierTransform::ReadPower(std::vector<fft_value_t>& power) {
    // ensure sufficient space
    if (power.size() != GetLengthPower()) {
        power.resize(GetLengthPower());
    }
    
    return ReadPower(&power[0]);
}
