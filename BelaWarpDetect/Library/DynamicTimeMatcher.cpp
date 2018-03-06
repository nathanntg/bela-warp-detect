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
#include <stdexcept>

DynamicTimeMatcher::DynamicTimeMatcher(const std::vector<const std::vector<float>> &templ) :
_length(templ.size()),
_features(templ[0].size()),
_tmpl(_features * _length),
_weight(_features * _length),
_alpha(_length),
_normalize(0),
_dpp_score(2 * (_length + 1)),
_dpp_len(2 * (_length + 1)) {
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
_length(length),
_features(features),
_tmpl(_features * _length),
_weight(_features * _length),
_alpha(_length),
_normalize(0),
_dpp_score(2 * (_length + 1)),
_dpp_len(2 * (_length + 1)) {
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

bool DynamicTimeMatcher::SetWeight(const float *weight) {
    // copy template features
    memcpy(_weight.ptr(), weight, sizeof(float) * _length * _features);
    
    // enable weights
    _use_weights = true;
    
    // calculate normalization
    _CalculateNormalize();
    
    // reset existing matches
    Reset();
    
    return true;
}

bool DynamicTimeMatcher::SetWeight(const std::vector<const std::vector<float>>& weight) {
    if (weight.size() != _length || weight[0].size() != _features) {
        return false;
    }
    
    // allocate template space
    for (unsigned int i = 0; i < _length; ++i) {
        // confirm matching number of features
        // (maybe throw exception or abort?)
        if (weight[i].size() != _features) {
            throw std::invalid_argument("requires constant sized feature vector");
        }
        
        // copy template features
        memcpy(_weight.ptr() + (i * _features), &weight[i][0], sizeof(float) * _features);
    }
    
    // enable weights
    _use_weights = true;
    
    // calculate normalization
    _CalculateNormalize();
    
    // reset existing matches
    Reset();
    
    return true;
}

void DynamicTimeMatcher::Reset() {
    // set index to first column of DPP
    _idx = 0;
    
    _dpp_score[0] = 0.0;
    _dpp_len[0] = 0;
    for (unsigned int i = 1; i < (_length + 1); ++i) {
        _dpp_score[i] = std::numeric_limits<float>::max();
        // dpp_len should not matter, since score is maxed out
    }
}

void DynamicTimeMatcher::_CalculateNormalize() {
    _normalize = 0.f;
    
    // compare with zeros
    // ALTERNATIVE IDEA: compare to scaled version of self
    ManagedMemory<float> zeros(_features);
    
    for (size_t i = 0; i < _length; ++i) {
        if (_use_weights) {
            _normalize += _ScoreFeaturesWeighted(_tmpl.ptr() + (i * _features), zeros.ptr(), _weight.ptr() + (i * _features));
        }
        else {
            _normalize += _ScoreFeatures(_tmpl.ptr() + (i * _features), zeros.ptr());
        }
    }
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
    float result = 0;
    float tt, ss;
    for (unsigned int i = 0; i < _features; ++i) {
        tt = tmpl_feature[i];
        ss = signal_feature[i];
        result += ((ss - tt) * (ss - tt));
    }
    return sqrt(result);
}

float DynamicTimeMatcher::_ScoreFeaturesWeighted(const float *tmpl_feature, const float *signal_feature, const float *weights) {
    float result = 0;
    float dd, ww;
    for (unsigned int i = 0; i < _features; ++i) {
        dd = tmpl_feature[i] - signal_feature[i];
        ww = weights[i];
        result += ww * dd * dd;
    }
    return sqrt(result);
}

struct dtm_out DynamicTimeMatcher::IngestFeatureVector(const float *features) {
    // error response
    struct dtm_out ret = {-1.0, -1.0, 0};
    
    // pointers to alternating DPP results
    float *_lst_score;
    unsigned int *_lst_len;
    float *_cur_score;
    unsigned int *_cur_len;
    if (_idx == 0) {
        _lst_score = _dpp_score.ptr();
        _lst_len = _dpp_len.ptr();
        _cur_score = _dpp_score.ptr() + (_length + 1);
        _cur_len = _dpp_len.ptr() + (_length + 1);
        _idx = 1;
    }
    else {
        _lst_score = _dpp_score.ptr() + (_length + 1);
        _lst_len = _dpp_len.ptr() + (_length + 1);
        _cur_score = _dpp_score.ptr();
        _cur_len = _dpp_len.ptr();
        _idx = 0;
    }
    
    // for each potential spot in the template
    float cost, alpha, score, t_score;
    unsigned int len;
    for (unsigned int i = 0; i < _length; ++i) {
        // current alpha
        alpha = _alpha[i];
        
        // current cost
        if (_use_weights) {
            cost = _ScoreFeaturesWeighted(_tmpl.ptr() + (i * _features), features, _weight.ptr() + (i * _features));
        }
        else {
            cost = _ScoreFeatures(_tmpl.ptr() + (i * _features), features);
        }
        
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
            
            // up (move in template space, but not in signal space)
            t_score = _cur_score[i] + cost * alpha;
            if (t_score < score) {
                score = t_score;
                len = _cur_len[i];
            }
            
            // left (move in signal space, not in template space)
            t_score = _lst_score[i + 1] + cost * alpha;
            if (t_score < score) {
                score = t_score;
                len = _lst_len[i + 1] + 1;
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
