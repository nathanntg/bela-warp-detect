//
//  MatchSyllables.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/20/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <limits>

#include "MatchSyllables.hpp"
#include "LoadAudio.hpp"

MatchSyllables::MatchSyllables(float sample_rate) :
_sample_rate(sample_rate),
_stft(_window_length, _window_stride, _buffer_length),
_power(_stft.GetLengthPower()),
_idx_lo(_stft.ConvertFrequencyToIndex(_freq_lo, _sample_rate)),
_idx_hi(_stft.ConvertFrequencyToIndex(_freq_hi, _sample_rate)) {
    
}

MatchSyllables::~MatchSyllables() {
    
}

void MatchSyllables::SetCallbackMatch(void (*cb)(int, float, int)) {
    _cb_match = cb;
}

void MatchSyllables::SetCallbackColumn(void (*cb)(std::vector<float>, std::vector<int>)) {
    _cb_column = cb;
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
    while (_stft.ReadPower(col)) {
        tmpl.push_back(std::vector<float>(col.begin() + _idx_lo, col.begin() + _idx_hi));
    }
    
    // add at end
    int index = _next_index++;
    _dtms.emplace_back(index, tmpl, threshold, constrain_length * (float)tmpl.size());
    
    return index;
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
        
        // flush matches
        for (auto it = _dtms.begin(); it != _dtms.end(); ++it) {
            it->dtm.Reset();
            it->last_score = std::numeric_limits<float>::max(); // resetting length does not matter
        }
    }
}

bool MatchSyllables::IngestAudio(const float *audio, const unsigned int len) {
    if (!_initialized) {
        return false;
    }
    
    // ingest values
    if (!_stft.WriteValues(audio, len)) {
        return false;
    }
    
    // match any
    _PerformMatching();
    
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
    
    // match any
    _PerformMatching();
    
    return true;
}

void MatchSyllables::_PerformMatching() {
    while (_stft.ReadPower(_power)) {
        for (auto it = _dtms.begin(); it != _dtms.end(); ++it) {
            struct dtm_out out = it->dtm.IngestFeatureVector(&_power[_idx_lo]);
        
            
            // was last time point below theshold, below length constraint and a local minimum?
            if (it->last_score < it->threshold && abs((float)it->last_len) < it->length && out.score > it->last_score) {
                // ALTERNATIVE:
                //if (out.score < _dtms[i].threshold && abs((float)out.len_diff) < _dtms[i].length) {
                //}
                
                // call match callback
                if (_cb_match) {
                    // trigger callback
                    _cb_match(it->index, it->last_score, it->last_len);
                }
                    
                // reset DTM? OR reset all?
                it->dtm.Reset();
                
                // max out score to prevent double trigger
                out.score = std::numeric_limits<float>::max();
            }
            
            // store last
            it->last_score = out.score;
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
    }
}
