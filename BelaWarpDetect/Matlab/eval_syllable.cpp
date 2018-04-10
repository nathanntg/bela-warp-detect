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

#define MX_TEST(TEST, ERR_ID, ERR_STR) if (!(TEST)) { mexErrMsgIdAndTxt(ERR_ID, ERR_STR); }

static bool testScalar(const mxArray *in) {
    // type
    if (!mxIsDouble(in) && !mxIsSingle(in)) {
        return false;
    }
    
    // real
    if (mxIsComplex(in)) {
        return false;
    }
    
    // dimensions
    if (mxGetNumberOfDimensions(in) != 2 || mxGetM(in) != 1 || mxGetN(in) != 1) {
        return false;
    }
    
    return true;
}

static double getScalar(const mxArray *in, const char *err_id, const char *err_str) {
    /* check scalar */
    if (!testScalar(in)) {
        mexErrMsgIdAndTxt(err_id, err_str);
    }
    
    /* get the scalar input */
    return mxGetScalar(in);
}

static bool testVector(const mxArray *in) {
    // type
    if (!mxIsDouble(in) && !mxIsSingle(in)) {
        return false;
    }
    
    // real
    if (mxIsComplex(in)) {
        return false;
    }
    
    // dimensions
    if (mxGetNumberOfDimensions(in) != 2) {
        return false;
    }
    
    // make sure there is at least one singleton dimension
    if (mxGetM(in) != 1 && mxGetN(in) != 1) {
        return false;
    }
    
    // make sure there is at least one non-zero dimension
    if (mxGetM(in) < 2 && mxGetN(in) < 2) {
        return false;
    }
    
    return true;
}

template <typename T> void getVector(const mxArray *in, std::vector<T> &vec, const char *err_id, const char *err_str) {
    // check vector
    if (!testVector(in)) {
        mexErrMsgIdAndTxt(err_id, err_str);
    }
    
    // allocate pointer
    size_t sn, sm, sl;
    
    // get dimensions
    sn = mxGetN(in);
    sm = mxGetM(in);
    sl = sn > 1 ? sn : sm;
    
    // resize vector
    vec.resize(sl);
    
    if (mxIsDouble(in)) {
        double *values = static_cast<double *>(mxGetPr(in));
        
        // fill vector
        for (size_t i = 0; i < sl; ++i) {
            vec[i] = static_cast<T>(values[i]);
        }
    }
    else if (mxIsSingle(in)) {
        float *values = reinterpret_cast<float *>(mxGetPr(in));
        
        // fill vector
        for (size_t i = 0; i < sl; ++i) {
            vec[i] = static_cast<T>(values[i]);
        }
    }
}

static bool testMatrix(const mxArray *in) {
    // type
    if (!mxIsDouble(in) && !mxIsSingle(in)) {
        return false;
    }
    
    // real
    if (mxIsComplex(in)) {
        return false;
    }
    
    // dimensions
    if (mxGetNumberOfDimensions(in) != 2) {
        return false;
    }
    
    return true;
}

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
        size_t sn, sm, sl;
        sm = mxGetM(prhs[0]);
        sn = mxGetN(prhs[0]);
        sl = sn * sm;
        ManagedMemory<float> mat(sn * sm);
        
        // fill matrix
        if (mxIsDouble(prhs[0])) {
            double *values = static_cast<double *>(mxGetPr(prhs[0]));
            for (size_t i = 0; i < sl; ++i) {
                mat[i] = static_cast<float>(values[i]);
            }
        }
        else if (mxIsSingle(prhs[0])) {
            float *values = reinterpret_cast<float *>(mxGetPr(prhs[0]));
            for (size_t i = 0; i < sl; ++i) {
                mat[i] = values[i];
            }
        }
        else {
            mexErrMsgIdAndTxt("MATLAB:es:invalidInput", "The syllable must be a real vector or matrix.");
        }
        
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
