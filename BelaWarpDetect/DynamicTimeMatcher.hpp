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

struct dtm_out {
    float score;
    unsigned int len; // length of match in signal space
};

class DynamicTimeMatcher
{
public:
    DynamicTimeMatcher();
    ~DynamicTimeMatcher();
    
    bool Initialize(const std::vector<std::vector<float>>& templ);
    bool SetAlpha(float alpha);
    bool SetAlpha(const std::vector<float>& alpha);
    
    void Reset();
    
    struct dtm_out IngestFeatureVector(const float *features);
    struct dtm_out IngestFeatureVector(const std::vector<float>& features);
    
private:
    float _ScoreFeatures(const float *tmpl_feature, const float *signal_feature);
    
    bool _initialized;
    
    unsigned int _features; // number of features in each step of the template
    unsigned int _length; // number of feature vectors in the template
    
    float *_alpha; // size = _features
    float *_tmpl; // size = _features * _length
    
    float *_dpp_score[2];
    unsigned int *_dpp_len[2];
    unsigned int _idx; // index in the dynamic plex propogation
};

#endif /* DynamicTimeMatcher_hpp */
