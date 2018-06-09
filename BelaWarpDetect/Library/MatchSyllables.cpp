//
//  MatchSyllables.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/20/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include "MatchSyllables.hpp"
#include "LoadAudio.hpp"
#include "ManagedMemory.hpp"

#include <cmath>

MatchSyllables::MatchSyllables(float sample_rate) :
_sample_rate(sample_rate),
_stft(_window_length, _window_stride, _buffer_length),
_idx_lo(_stft.ConvertFrequencyToIndex(_freq_lo, _sample_rate)),
_idx_hi(_stft.ConvertFrequencyToIndex(_freq_hi, _sample_rate)),
_features(_stft.GetLengthPower()) {
    _stft.SetWindowHanning();
}

MatchSyllables::~MatchSyllables() {
    
}

void MatchSyllables::SetCallbackMatch(void (*cb)(size_t, float, int)) {
    _cb_match = cb;
}

void MatchSyllables::SetCallbackColumn(void (*cb)(std::vector<float>, std::vector<int>)) {
    _cb_column = cb;
}

bool MatchSyllables::_ReadFeatures(std::vector<float> &features) {
    if (!_stft.ReadPower(features)) {
        return false;
    }
    
    // log?
    if (_log_power) {
        // potentially only calculate log within indices of interest
        for (auto it = features.begin(); it != features.end(); ++it) {
            *it = log(1.f + *it);
        }
    }
    
    return true;
}

int MatchSyllables::AddSyllable(const std::vector<float> &audio, float threshold, float constrain_length) {
    // clear STFT
    _stft.Clear();
    
    // write values
    if (!_stft.WriteValues(audio)) {
        return -1;
    }
    
    // zero pad to edge
    _stft.ZeroPadToEdge();
    
    // template for syllable
    std::vector<std::vector<float>> tmpl;
    
    // read in template
    std::vector<float> col;
    while (_ReadFeatures(col)) {
        tmpl.push_back(std::vector<float>(col.begin() + _idx_lo, col.begin() + _idx_hi));
    }
    
    // add at end
    size_t index = _next_index++;
    _dtms.emplace_back(index, tmpl, threshold, constrain_length * static_cast<float>(tmpl.size()));
    
    return static_cast<int>(index);
}

int MatchSyllables::AddSyllable(const std::string file, float threshold, float constrain_length) {
    // can not add syllables after initialization
    if (_initialized) {
        return -1;
    }
    
    std::vector<float> audio;
    float sample_rate;
    
    // load audio
    if (!LoadAudio(file, audio, sample_rate)) {
        return -1;
    }
    if (sample_rate != _sample_rate) {
        return -1;
    }
    
    return AddSyllable(audio, threshold, constrain_length);
}

int MatchSyllables::AddSpectrogram(const std::vector<std::vector<float>> &spect, float threshold, float constrain_length) {
    // invalid feature length
    if ((_idx_hi - _idx_lo) != spect[0].size()) {
        return -1;
    }
    
    // add at end
    size_t index = _next_index++;
    _dtms.emplace_back(index, spect, threshold, constrain_length * static_cast<float>(spect.size()));
    
    return static_cast<int>(index);
}

int MatchSyllables::AddSpectrogram(const float *spect, size_t length, size_t features, float threshold, float constrain_length) {
    // invalid feature length
    if ((_idx_hi - _idx_lo) != features) {
        return -1;
    }
    
    // add at end
    size_t index = _next_index++;
    _dtms.emplace_back(index, spect, length, features, threshold, constrain_length * static_cast<float>(length));
    
    return static_cast<int>(index);
}

int MatchSyllables::AddSpectrogram(const std::string file, float threshold, float constrain_length) {
    FILE *fh;
    fh = fopen(file.c_str(), "r");
    if (!fh) {
        return -1;
    }
    
    // obtain file size:
    fseek(fh, 0, SEEK_END);
    size_t file_length = ftell(fh);
    rewind(fh);
    
    // calculate size
    size_t features = _idx_hi - _idx_lo;
    size_t length = file_length / sizeof(float) / features;
    size_t total = features * length;
    if (file_length != total * sizeof(float)) {
        return -1;
    }
    
    // read file
    ManagedMemory<float> buffer(total);
    if (total != fread(static_cast<void *>(buffer.ptr()), sizeof(float), total, fh)) {
        return -1;
    }
    
    // close file
    fclose(fh);
    
    // add at end
    size_t index = _next_index++;
    _dtms.emplace_back(index, buffer.ptr(), length, features, threshold, constrain_length * static_cast<float>(length));
    
    return static_cast<int>(index);
}

bool MatchSyllables::Initialize() {
    if (_initialized) {
        return false;
    }
    if (_dtms.empty()) {
        return false;
    }
    
    // set initialize
    _initialized = true;
    
    // reset
    Reset();
    
    return true;
}

void MatchSyllables::Reset() {
    if (_initialized) {
        // clear circular buffer
        _stft.Clear();
        
        // clear flag
        _match_state = 0;
        
        // flush matches
        for (auto it = _dtms.begin(); it != _dtms.end(); ++it) {
            it->dtm.Reset();
            it->last_score = 0.f;
        }
    }
}

bool MatchSyllables::IngestAudio(const float *audio, const unsigned int len, const unsigned int stride) {
    if (!_initialized) {
        return false;
    }
    
    // ingest values
    if (!_stft.WriteValues(audio, len, stride)) {
        return false;
    }
    
    return true;
}

bool MatchSyllables::IngestAudio(const std::vector<float> &audio) {
    if (!_initialized) {
        return false;
    }
    
    // ingest values
    if (!_stft.WriteValues(audio)) {
        return false;
    }
    
    return true;
}

bool MatchSyllables::MatchOnce(float *score, int *len) {
    // enough data to read features?
    if (!_ReadFeatures(_features)) {
        return false;
    }
    
    if (0 != _match_state) {
        float sum = 0.;
        for (size_t i = _idx_lo; i < _idx_hi; ++i) {
            sum += _features[i];
        }
        
        // power decreasing
        if (sum < 16.) {
            // trigger feedback
            if (_cb_match) {
                _cb_match(_next_index, 0, 0);
            }
            
            // reset state
            _match_state = 0;
        }
        
        if (_cb_column) {
            std::vector<float> scores = std::vector<float>(_next_index);
            std::vector<int> lengths = std::vector<int>(_next_index);
            for (auto it = _dtms.begin(); it != _dtms.end(); ++it) {
                scores[it->index] = (0 == _match_state ? 1. : 0.);
                lengths[it->index] = 0;
            }
            _cb_column(scores, lengths);
        }
        
        return true;
    }
    
    for (auto it = _dtms.begin(); it != _dtms.end(); ++it) {
        struct dtm_out out = it->dtm.IngestFeatureVector(&_features[_idx_lo]);
        
        // was last time point below theshold, below length constraint and a local minimum?
        if (it->last_score >= it->threshold && fabs(static_cast<float>(it->last_len)) < it-> threshold_length && out.normalized_score < it->last_score) {
            // ALTERNATIVE:
            //if (out.score >= _dtms[i].threshold && abs((float)out.len_diff) < _dtms[i].length) {
            //}
            
            // call match callback
            if (_cb_match) {
                // trigger callback
                _cb_match(it->index, it->last_score, it->last_len);
            }
            
            // set match state
            _match_state = 1;
            
            // reset DTM? OR reset all?
            it->dtm.Reset();
            
            // zero out score to prevent double trigger
            out.normalized_score = 0.f;
        }
        
        // populate arguments
        if (score) {
            score[it->index] = out.normalized_score;
        }
        if (len) {
            len[it->index] = out.len_diff;
        }
        
        // store last
        it->last_score = out.normalized_score;
        it->last_len = out.len_diff;
    }
    
    // call at end of each column
    if (_cb_column) {
        std::vector<float> scores = std::vector<float>(_next_index);
        std::vector<int> lengths = std::vector<int>(_next_index);
        for (auto it = _dtms.begin(); it != _dtms.end(); ++it) {
            scores[it->index] = it->last_score;
            lengths[it->index] = it->last_len;
        }
        _cb_column(scores, lengths);
    }
    
    return true;
}

void MatchSyllables::PerformMatching() {
    while (MatchOnce(NULL, NULL));
}

bool MatchSyllables::ZeroPadAndFetch(std::vector<float> &scores, std::vector<int> &lengths) {
    if (!_initialized) {
        return false;
    }
    
    // zero pad to edge
    _stft.ZeroPadToEdge();
    
    // perform matching
    PerformMatching();
    
    // resize returns
    scores.resize(_next_index);
    lengths.resize(_next_index);
    
    // fill last score and last length
    for (auto it = _dtms.begin(); it != _dtms.end(); ++it) {
        scores[it->index] = it->last_score;
        lengths[it->index] = it->last_len;
    }
    
    return true;
}
