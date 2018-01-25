//
//  DynamicTimeMatcher.hpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/15/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#ifndef DynamicTimeMatcher_hpp
#define DynamicTimeMatcher_hpp

#include <stdio.h>
#include <vector>

#include "ManagedMemory.hpp"

struct dtm_out {
    float score;
    int len_diff; // length of match in signal space
};

class DynamicTimeMatcher
{
public:
    DynamicTimeMatcher(const std::vector<std::vector<float>> &templ);
    ~DynamicTimeMatcher();
    
    bool SetAlpha(float alpha);
    bool SetAlpha(const std::vector<float>& alpha);
    
    void Reset();
    
    struct dtm_out IngestFeatureVector(const float *features);
    struct dtm_out IngestFeatureVector(const std::vector<float>& features);
    
private:
    float _ScoreFeatures(const float *tmpl_feature, const float *signal_feature);
    
    size_t _features; // number of features in each step of the template
    size_t _length; // number of feature vectors in the template
    
    ManagedMemory<float> _alpha; // size = _features
    ManagedMemory<float> _tmpl; // size = _features * _length
    
    ManagedMemory<float> _dpp_score;
    ManagedMemory<unsigned int> _dpp_len;
    unsigned int _idx; // index in the dynamic plex propogation
};

#endif /* DynamicTimeMatcher_hpp */
