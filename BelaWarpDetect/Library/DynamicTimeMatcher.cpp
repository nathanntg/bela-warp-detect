//
//  DynamicTimeMatcher.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/15/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include "DynamicTimeMatcher.hpp"

#include <limits>
#include <cmath>
#include <cstring> // memcpy
#include <stdexcept>
#include <math.h> // isnan

DynamicTimeMatcher::DynamicTimeMatcher(const std::vector<std::vector<float>> &templ) :
_features(templ[0].size()),
_length(templ.size()),
_tmpl(_features * _length),
_alpha(_length),
_normalize(0),
_dpp_score(3 * (_length + 1)),
_dpp_len(3 * (_length + 1)) {
    // require non-zero length and feature size
    if (templ.empty() || templ[0].empty()) {
        throw std::invalid_argument("requires non-empty template with non-empty feature vector");
    }
    
    // allocate template space
    for (unsigned int i = 0; i < _length; ++i) {
        // confirm matching number of features
        // (maybe throw exception or abort?)
        if (templ[i].size() != _features) {
            throw std::invalid_argument("requires constant sized feature vector");
        }
        
        // copy template features
        memcpy(_tmpl.ptr() + (i * _features), &templ[i][0], sizeof(float) * _features);
    }
    
    // calculate normalization
    _CalculateNormalize();
    
    // allocate alpha
    SetAlpha(1.0);
    
    // reset dpp storage
    Reset();
}

DynamicTimeMatcher::DynamicTimeMatcher(const float *templ, size_t length, size_t features) :
_features(features),
_length(length),
_tmpl(_features * _length),
_alpha(_length),
_normalize(0),
_dpp_score(3 * (_length + 1)),
_dpp_len(3 * (_length + 1)) {
    // copy template features
    memcpy(_tmpl.ptr(), templ, sizeof(float) * length * features);
    
    // calculate normalization
    _CalculateNormalize();
    
    // allocate alpha
    SetAlpha(1.0);
    
    // reset dpp storage
    Reset();
}

DynamicTimeMatcher::~DynamicTimeMatcher() {
    
}

bool DynamicTimeMatcher::SetAlpha(float alpha) {
    // set alpha
    for (unsigned int i = 0; i < _length; ++i) {
        _alpha[i] = alpha;
    }
    
    return true;
}

bool DynamicTimeMatcher::SetAlpha(const std::vector<float>& alpha) {
    // check alpha length
    if (alpha.size() != _length) {
        return false;
    }
    
    // set alpha
    for (unsigned int i = 0; i < _length; ++i) {
        _alpha[i] = alpha[i];
    }
    
    return true;
}

void DynamicTimeMatcher::Reset() {
    // set index to first column of DPP
    _idx = 0;
    
    _dpp_score[0] = 0.0;
    _dpp_len[0] = 0;
    for (unsigned int i = 1; i < _dpp_score.size(); ++i) {
        _dpp_score[i] = std::numeric_limits<float>::max();
        // dpp_len should not matter, since score is maxed out
    }
}

void DynamicTimeMatcher::_CalculateNormalize() {
    _normalize = 0.5 * static_cast<float>(_length);
}

float DynamicTimeMatcher::_NormalizeScore(float score) {
    float normalized = (_normalize - score) / _normalize;
    if (normalized < 0.f) {
        return 0.f;
    }
    if (normalized > 1.f) {
        return 1.f;
    }
    return normalized;
}

float DynamicTimeMatcher::_ScoreFeatures(const float *tmpl_feature, const float *signal_feature) {
    float dot = 0;
    float norm_t = 0;
    float norm_s = 0;
    
    float tt, ss;
    for (unsigned int i = 0; i < _features; ++i) {
        // get current entries
        tt = tmpl_feature[i];
        ss = signal_feature[i];
        
        // update counts
        dot += tt * ss;
        norm_t += tt * tt;
        norm_s += ss * ss;
    }
    
    // check power?
    if (norm_t < 0.5 && norm_s < 0.5) {
        return 0.f;
    }
    
    float result = dot / (sqrt(norm_t) * sqrt(norm_s));
    return 1.f - result;
}

struct dtm_out DynamicTimeMatcher::IngestFeatureVector(const float *features) {
    // error response
    struct dtm_out ret = {-1.0, -1.0, 0};
    
    // pointers to alternating DPP results
    float *_lstlst_score;
    unsigned int *_lstlst_len;
    float *_lst_score;
    unsigned int *_lst_len;
    float *_cur_score;
    unsigned int *_cur_len;
    if (_idx == 0) {
        _lst_score = _dpp_score.ptr();
        _lst_len = _dpp_len.ptr();
        _cur_score = _dpp_score.ptr() + (_length + 1);
        _cur_len = _dpp_len.ptr() + (_length + 1);
        _lstlst_score = _dpp_score.ptr() + (_length + 1 + _length + 1);
        _lstlst_len = _dpp_len.ptr() + (_length + 1 + _length + 1);
        _idx = 1;
    }
    else if (_idx == 1) {
        _lst_score = _dpp_score.ptr() + (_length + 1);
        _lst_len = _dpp_len.ptr() + (_length + 1);
        _cur_score = _dpp_score.ptr() + (_length + 1 + _length + 1);
        _cur_len = _dpp_len.ptr() + (_length + 1 + _length + 1);
        _lstlst_score = _dpp_score.ptr();
        _lstlst_len = _dpp_len.ptr();
        _idx = 2;
    }
    else {
        _lst_score = _dpp_score.ptr() + (_length + 1 + _length + 1);
        _lst_len = _dpp_len.ptr() + (_length + 1 + _length + 1);
        _cur_score = _dpp_score.ptr();
        _cur_len = _dpp_len.ptr();
        _lstlst_score = _dpp_score.ptr() + (_length + 1);
        _lstlst_len = _dpp_len.ptr() + (_length + 1);
        _idx = 0;
    }
    
    // for each potential spot in the template
    float cost, alpha, score, t_score;
    unsigned int len;
    for (unsigned int i = 0; i < _length; ++i) {
        // current alpha
        alpha = _alpha[i];
        
        // current cost
        cost = _ScoreFeatures(_tmpl.ptr() + (i * _features), features);
        
        // is nan? (special case)
        if (isnan(cost)) {
            // assume diagonal
            score = _lst_score[i];
            len = _lst_len[i] + 1;
        }
        else {
            // diagonal (move in both template and signal space)
            score = _lst_score[i] + cost;
            len = _lst_len[i] + 1;
            
            // up (move 2 in template space, and 1 in signal space)
            if (i > 0) {
                t_score = _lst_score[i - 1] + cost * alpha;
                if (t_score < score) {
                    score = t_score;
                    len = _lst_len[i - 1] + 1;
                }
            }
            
            // left (move 2 in signal space, and 1 in template space)
            t_score = _lstlst_score[i] + cost * alpha;
            if (t_score < score) {
                score = t_score;
                len = _lstlst_len[i] + 2;
            }
        }
        
        _cur_score[i + 1] = score;
        _cur_len[i + 1] = len;
    }
    
    ret.score = _cur_score[_length];
    ret.normalized_score = _NormalizeScore(_cur_score[_length]);
    ret.len_diff = static_cast<int>(_cur_len[_length]) - static_cast<int>(_length);
    
    return ret;
}

struct dtm_out DynamicTimeMatcher::IngestFeatureVector(const std::vector<float>& features) {
    return IngestFeatureVector(&features[0]);
}
