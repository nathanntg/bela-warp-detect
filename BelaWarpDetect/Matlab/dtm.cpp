//
//  dtm.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/16/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <iostream>
#include <vector>
#include <mex.h>

#include "Library/CircularShortTimeFourierTransform.hpp"
#include "Library/DynamicTimeMatcher.hpp"
#include "Matlab/Matlab.hpp"

/* the gateway function */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    std::vector<float> signal, templ;
    float sampling_rate;
    
    /* check for proper number of arguments */
    MX_TEST(nrhs == 3, "MATLAB:dtm:invalidNumInputs", "Requires three inputs (template, signal, sampling rate).");
    MX_TEST(nlhs == 2, "MATLAB:dtm:invalidNumOutputs", "Requires two outputs (scores and lengths).");
    
    /* read arguments */
    getVector(prhs[0], templ, "MATLAB:dtm:invalidInput", "The template must be a real vector.");
    getVector(prhs[1], signal, "MATLAB:dtm:invalidInput", "The signal must be a real vector.");
    sampling_rate = static_cast<float>(getScalar(prhs[2], "MATLAB:dtm:invalidInput", "The sampling rate must be a real scalar."));
    
    /* parameters */
    unsigned int stft_len = 512, stft_stride = 40;
    double freq_lo = 1e3, freq_hi = 9e3;
    
    /* create STFT */
    CircularShortTermFourierTransform stft(stft_len, stft_stride, signal.size() + stft_len);
    unsigned int idx_lo = stft.ConvertFrequencyToIndex(freq_lo, sampling_rate);
    unsigned int idx_hi = stft.ConvertFrequencyToIndex(freq_hi, sampling_rate);
    
    /* allocate space for outputs */
    unsigned int len_feature = stft.GetLengthPower();
    unsigned int len_templ = stft.ConvertSamplesToColumns(templ.size());
    unsigned int len_signal = stft.ConvertSamplesToColumns(signal.size());
    
    /* generate template */
    stft.WriteValues(templ);
    std::vector<float> col;
    std::vector<std::vector<float>> features_templ;
    for (unsigned int i = 0; i < len_templ; ++i) {
        if (!stft.ReadPower(col)) {
            mexErrMsgIdAndTxt("MATLAB:dtm:internalError", "Unable to generate expected number of spectral columns for the template.");
        }
        
        // slice, slow
        features_templ.push_back(std::vector<float>(col.begin() + idx_lo, col.begin() + idx_hi));
    }
    
    /* make matcher */
    DynamicTimeMatcher dtm(features_templ);
    
    /* generate outputs */
    double *scores;
    double *lengths;
    plhs[0] = mxCreateDoubleMatrix(1, len_signal, mxREAL);
    scores = mxGetPr(plhs[0]);
    plhs[1] = mxCreateDoubleMatrix(1, len_signal, mxREAL);
    lengths = mxGetPr(plhs[1]);
    
    /* process signal */
    stft.Clear();
    stft.WriteValues(signal);
    for (unsigned int i = 0; i < len_signal; ++i) {
        if (!stft.ReadPower(col)) {
            mexErrMsgIdAndTxt("MATLAB:dtm:internalError", "Unable to generate expected number of spectral columns for the signal.");
        }
        
        // result
        auto result = dtm.IngestFeatureVector(std::vector<float>(col.begin() + idx_lo, col.begin() + idx_hi));
        
        // append to results
        scores[i] = static_cast<double>(result.score);
        lengths[i] = static_cast<double>(result.len_diff);
    }
}
