//
//  MatchSyllables.hpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/20/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#ifndef MatchSyllables_hpp
#define MatchSyllables_hpp

#include <stdio.h>
#include <string>
#include <vector>
#include <list>
#include <limits>

#include "CircularShortTimeFourierTransform.hpp"
#include "DynamicTimeMatcher.hpp"

struct ms_dtm {
    int index;
    DynamicTimeMatcher dtm;
    float threshold;
    float length;
    float last_score;
    int last_len;
    
    ms_dtm(int index_, const std::vector<std::vector<float>> &tmpl, float threshold_, float length_) : index(index_), dtm(tmpl), threshold(threshold_), length(length_), last_score(std::numeric_limits<float>::max()), last_len(0) {
        
    }
};

class MatchSyllables
{
public:
    MatchSyllables(float sample_rate);
    ~MatchSyllables();
    
    // returns a syllable ID, used when identifying
    int AddSyllable(const std::vector<float> &audio, float threshold, float constrain_length = 0.07);
    int AddSyllable(const std::string file, float threshold, float constrain_length = 0.07);
    
    void SetCallbackMatch(void (*cb)(int, float, int));
    void SetCallbackColumn(void (*cb)(std::vector<float>, std::vector<int>)); // for debugging purposes, called once per syllable per column
    
    // initialize (callbacks and syllables can no longer be added)
    bool Initialize();
    
    // reset audio buffer and matchers
    void Reset();
    
    // ingest new audio
    bool IngestAudio(const float *audio, const unsigned int len);
    bool IngestAudio(const std::vector<float>& audio);
    
private:
    // perform matching
    void _PerformMatching();
    
    bool _initialized = false;
    
    // next index
    int _next_index = 0;
    
    // sampling rate for the audio
    const float _sample_rate;
    
    // parameters
    const unsigned int _buffer_length = 2097152;
    const unsigned int _window_length = 512;
    const unsigned int _window_stride = 40;
    const float _freq_lo = 1000.0;
    const float _freq_hi = 10000.0;
    
    // circular short term fourier transform
    CircularShortTermFourierTransform _stft;
    
    // indices of frequencies in STFT
    const unsigned int _idx_lo;
    const unsigned int _idx_hi;
    
    std::vector<float> _power;
    
    // vector of matchers
    std::list<struct ms_dtm> _dtms;
    
    // callback
    void (*_cb_match)(int, float, int);
    void (*_cb_column)(std::vector<float>, std::vector<int>);
};

#endif /* MatchSyllables_hpp */
