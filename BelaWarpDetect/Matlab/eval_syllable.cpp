//
//  eval_syllable.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/30/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <stdio.h>

#include <iostream>
#include <vector>
#include <mex.h>

#include "Library/ManagedMemory.hpp"
#include "Library/MatchSyllables.hpp"
#include "Matlab/Matlab.hpp"

/* the gateway function */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    float sampling_rate;
    std::vector<float> syllable;
    
    /* check for proper number of arguments */
    MX_TEST(nrhs >= 3, "MATLAB:es:invalidNumInputs", "Requires three or more inputs (syllable, sampling rate and one or more syllables to evaluate)");
    MX_TEST(nlhs == 2, "MATLAB:es:invalidNumOutputs", "Requires two outputs (scores and lengths).");
    
    /* read arguments */
    sampling_rate = static_cast<float>(getScalar(prhs[1], "MATLAB:es:invalidInput", "The sampling rate must be a real scalar."));
    
    /* create matcher */
    MatchSyllables ms(sampling_rate);
    
    /* add syllable */
    if (testVector(prhs[0])) {
        getVector(prhs[0], syllable, "MATLAB:es:invalidInput", "The syllable must be a real vector or matrix.");
        if (ms.AddSyllable(syllable, 1e6) == -1) {
            mexErrMsgIdAndTxt("MATLAB:es:internalError", "Unable to add the syllable to the matcher.");
        }
    }
    else if (testMatrix(prhs[0])) {
        size_t sn, sm;
        ManagedMemory<float> mat(getMatrix<float>(prhs[0], sm, sn, "MATLAB:es:invalidInput", "The syllable must be a real vector or matrix."));
        if (ms.AddSpectrogram(mat.ptr(), sn, sm, 1e6) == -1) {
            mexErrMsgIdAndTxt("MATLAB:es:internalError", "Unable to add the syllable to the matcher.");
        }
    }
    else {
        mexErrMsgIdAndTxt("MATLAB:es:invalidInput", "The syllable must be a real vector or matrix.");
    }
    
    /* initialize */
    if (!ms.Initialize()) {
        mexErrMsgIdAndTxt("MATLAB:es:internalError", "Unable to initialize syllable matcher.");
    }
    
    // generate outputs
    size_t results = nrhs - 2;
    
    double *scores;
    double *lengths;
    plhs[0] = mxCreateDoubleMatrix(1, results, mxREAL);
    scores = mxGetPr(plhs[0]);
    plhs[1] = mxCreateDoubleMatrix(1, results, mxREAL);
    lengths = mxGetPr(plhs[1]);
    
    // for each syllabe to evaluate
    std::vector<float> syllable_to_eval;
    std::vector<float> result_score(1);
    std::vector<int> result_length(1);
    for (size_t i = 2; i < nrhs; ++i) {
        // get input
        getVector(prhs[i], syllable_to_eval, "MATLAB:es:invalidInput", "The syllable to evaluate must be a real vector.");
        
        // reset
        ms.Reset();
        
        // ingest
        if (!ms.IngestAudio(syllable_to_eval)) {
            mexErrMsgIdAndTxt("MATLAB:es:internalError", "IngestAudio failed.");
        }
        
        // zero pad and fetch output
        if (!ms.ZeroPadAndFetch(result_score, result_length)) {
            mexErrMsgIdAndTxt("MATLAB:es:internalError", "ZeroPadAndFetch failed.");
        }
        
        scores[i - 2] = static_cast<double>(result_score[0]);
        lengths[i - 2] = static_cast<double>(result_length[0]);
    }
}
