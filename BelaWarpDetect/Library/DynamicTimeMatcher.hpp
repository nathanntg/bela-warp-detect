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
    float normalized_score; // 0 to 1, 1 is a better match (more intuitive than)
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
    
    float GetNormalize() { return _normalize; }
    
    size_t GetFeatures() { return _features; }
    size_t GetLength() { return _length; }
    
    struct dtm_out IngestFeatureVector(const float *features);
    struct dtm_out IngestFeatureVector(const std::vector<float>& features);
    
private:
    void _CalculateNormalize();
    float _NormalizeScore(float score);
    
    float _ScoreFeatures(const float *tmpl_feature, const float *signal_feature);
    
    size_t _features; // number of features in each step of the template
    size_t _length; // number of feature vectors in the template
    
    ManagedMemory<float> _tmpl; // size = _features * _length
    ManagedMemory<float> _alpha; // size = _length
    
    float _normalize; // normalization that allows comparing across DynamicTimeMatcher instances
    
    ManagedMemory<float> _dpp_score;
    ManagedMemory<unsigned int> _dpp_len;
    unsigned int _idx; // index in the dynamic plex propogation
};

#endif /* DynamicTimeMatcher_hpp */
