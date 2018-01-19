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

DynamicTimeMatcher::DynamicTimeMatcher() {
    _initialized = false;
}

DynamicTimeMatcher::~DynamicTimeMatcher() {
    if (_initialized) {
        free(_alpha);
        free(_tmpl);
        
        free(_dpp_len[0]);
        free(_dpp_len[1]);
        
        free(_dpp_score[0]);
        free(_dpp_score[1]);
    }
}

bool DynamicTimeMatcher::Initialize(const std::vector<std::vector<float>> &templ) {
    // already initialized?
    if (_initialized) {
        return false;
    }
    
    // require non-zero length and feature size
    if (templ.empty() || templ[0].empty()) {
        return false;
    }
    
    // store features and length
    _length = templ.size();
    _features = templ[0].size();
    
    // allocate template space
    _tmpl = (float *)malloc(sizeof(float) * _features * _length);
    for (unsigned int i = 0; i < _length; ++i) {
        // confirm matching number of features
        // (maybe throw exception or abort?)
        if (templ[i].size() != _features) {
            free(_tmpl);
            return false;
        }
        
        // copy template features
        memcpy(_tmpl + (i * _features), &templ[i][0], sizeof(float) * _features);
    }
    
    // allocate alpha space
    _alpha = (float *)malloc(sizeof(float) * _length);
    
    // allocate dpp score storage space
    _dpp_score[0] = (float *)calloc(_length + 1, sizeof(float));
    _dpp_score[1] = (float *)calloc(_length + 1, sizeof(float));
    
    // allocate dpp length storage space
    _dpp_len[0] = (unsigned int *)calloc(_length + 1, sizeof(unsigned int));
    _dpp_len[1] = (unsigned int *)calloc(_length + 1, sizeof(unsigned int));
    
    // success
    _initialized = true;
    
    
    // allocate alpha
    SetAlpha(1.0);
    
    // reset dpp storage
    Reset();
    
    return true;
}

bool DynamicTimeMatcher::SetAlpha(float alpha) {
    // not initialized
    if (!_initialized) {
        return false;
    }
    
    // set alpha
    for (unsigned int i = 0; i < _length; ++i) {
        _alpha[i] = alpha;
    }
    
    return true;
}

bool DynamicTimeMatcher::SetAlpha(const std::vector<float>& alpha) {
    // not initialized
    if (!_initialized) {
        return false;
    }
    
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
    // not initialized
    if (!_initialized) {
        return;
    }
    
    // set index to first column of DPP
    _idx = 0;
    
    _dpp_score[0][0] = 0.0;
    _dpp_len[0][0] = 0;
    for (unsigned int i = 1; i < (_length + 1); ++i) {
        _dpp_score[0][i] = std::numeric_limits<float>::max();
        // dpp_len should not matter, since score is maxed out
    }
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

struct dtm_out DynamicTimeMatcher::IngestFeatureVector(const float *features) {
    // error response
    struct dtm_out ret = {-1.0, 0};
    
    // not initialized
    if (!_initialized) {
        return ret;
    }
    
    // pointers to alternating DPP results
    float *_lst_score;
    unsigned int *_lst_len;
    float *_cur_score;
    unsigned int *_cur_len;
    if (_idx == 0) {
        _lst_score = _dpp_score[0];
        _lst_len = _dpp_len[0];
        _cur_score = _dpp_score[1];
        _cur_len = _dpp_len[1];
        _idx = 1;
    }
    else {
        _lst_score = _dpp_score[1];
        _lst_len = _dpp_len[1];
        _cur_score = _dpp_score[0];
        _cur_len = _dpp_len[0];
        _idx = 0;
    }
    
    // for each potential spot in the template
    float cost, alpha, score, t_score;
    unsigned int len;
    for (unsigned int i = 0; i < _length; ++i) {
        // current alpha
        alpha = _alpha[i];
        
        // current cost
        cost = _ScoreFeatures(_tmpl + (i * _features), features);
        
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
    ret.len_diff = (int)_cur_len[_length] - (int)_length;
    
    return ret;
}

struct dtm_out DynamicTimeMatcher::IngestFeatureVector(const std::vector<float>& features) {
    return IngestFeatureVector(&features[0]);
}
