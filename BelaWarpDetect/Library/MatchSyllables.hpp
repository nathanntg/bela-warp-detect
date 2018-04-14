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

#include "CircularShortTimeFourierTransform.hpp"
#include "DynamicTimeMatcher.hpp"

struct ms_dtm {
    size_t index;
    DynamicTimeMatcher dtm;
    float threshold;
    float threshold_length;
    float last_score;
    int last_len;
    
    ms_dtm(size_t index_, const std::vector<std::vector<float>> &tmpl, float threshold_, float threshold_length_) : index(index_), dtm(tmpl), threshold(threshold_), threshold_length(threshold_length_), last_score(0.f), last_len(0) {
        float a;
        size_t dtm_length = dtm.GetLength();
        std::vector<float> alpha(dtm_length);
        for (size_t i = 0, maxi = (dtm_length / 2) + 1; i < maxi; ++i) {
            a = 2.f + 1.f * pow(0.9f, static_cast<float>(i));
            alpha[i] = a;
            alpha[dtm_length - 1 - i] = a;
        }
        
        dtm.SetAlpha(alpha);
    }
    
    ms_dtm(size_t index_, const float *tmpl, size_t length, size_t features, float threshold_, float threshold_length_) : index(index_), dtm(tmpl, length, features), threshold(threshold_), threshold_length(threshold_length_), last_score(0.f), last_len(0) {
        float a;
        size_t dtm_length = dtm.GetLength();
        std::vector<float> alpha(dtm_length);
        for (size_t i = 0, maxi = (dtm_length / 2) + 1; i < maxi; ++i) {
            a = 2.f + 1.f * pow(0.9f, static_cast<float>(i));
            alpha[i] = a;
            alpha[dtm_length - 1 - i] = a;
        }
        
        dtm.SetAlpha(alpha);
    }
};

class MatchSyllables
{
public:
    MatchSyllables(float sample_rate);
    ~MatchSyllables();
    
    // returns a syllable ID, used when identifying
    int AddSyllable(const std::vector<float> &audio, float threshold, float constrain_length = 0.25f);
    int AddSyllable(const std::string file, float threshold, float constrain_length = 0.25f);
    
    int AddSpectrogram(const std::vector<std::vector<float>> &spect, float threshold, float constrain_length = 0.25f);
    int AddSpectrogram(const float *spect, size_t length, size_t features, float threshold, float constrain_length = 0.25f);
    int AddSpectrogram(const std::string file, float threshold, float constrain_length = 0.25f);
    
    void SetCallbackMatch(void (*cb)(size_t, float, int));
    void SetCallbackColumn(void (*cb)(std::vector<float>, std::vector<int>)); // for debugging purposes, called once per syllable per column
    
    // initialize (callbacks and syllables can no longer be added)
    bool Initialize();
    
    // reset audio buffer and matchers
    void Reset();
    
    // ingest new audio
    bool IngestAudio(const float *audio, const unsigned int len, const unsigned int stride=1);
    bool IngestAudio(const std::vector<float>& audio);
    
    // perform matching
    bool MatchOnce(float *score, int *len);
    void PerformMatching();
    
    // debugging option
    bool ZeroPadAndFetch(std::vector<float> &scores, std::vector<int> &lengths);
    
private:
    // perform matching
    bool _ReadFeatures(std::vector<float> &power);
    
    bool _initialized = false;
    
    // next index
    size_t _next_index = 0;
    
    // sampling rate for the audio
    const float _sample_rate;
    
    // parameters
    const unsigned int _buffer_length = 2097152;
    const unsigned int _window_length = 512;
    const unsigned int _window_stride = 40;
    const float _freq_lo = 1000.0f;
    const float _freq_hi = 10000.0f;
    const bool _log_power = true;
    
    // circular short term fourier transform
    CircularShortTermFourierTransform _stft;
    
    // indices of frequencies in STFT
    const unsigned int _idx_lo;
    const unsigned int _idx_hi;
    
    // current feature column for matching (power)
    std::vector<float> _features;
    
    // vector of matchers
    std::list<struct ms_dtm> _dtms;
    
    // callback
    void (*_cb_match)(size_t, float, int) = nullptr;
    void (*_cb_column)(std::vector<float>, std::vector<int>) = nullptr;
};

#endif /* MatchSyllables_hpp */
