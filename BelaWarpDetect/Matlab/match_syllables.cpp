//
//  match_syllables.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/20/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <iostream>
#include <vector>
#include <mex.h>

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

std::vector<std::vector<float>> result_score;
std::vector<std::vector<int>> result_length;

void cbAppendResult(std::vector<float> scores, std::vector<int> lengths) {
    result_score.push_back(scores);
    result_length.push_back(lengths);
}


/* the gateway function */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    float sampling_rate;
    std::vector<float> signal;
    
    /* check for proper number of arguments */
    MX_TEST(nrhs >= 3, "MATLAB:ms:invalidNumInputs", "Requires three or more inputs (signal, sampling rate and one or more syllables)");
    MX_TEST(nlhs == 2, "MATLAB:ms:invalidNumOutputs", "Requires two outputs (scores and lengths).");
    
    /* read arguments */
    getVector(prhs[0], signal, "MATLAB:ms:invalidInput", "The signal must be a real vector.");
    sampling_rate = static_cast<float>(getScalar(prhs[1], "MATLAB:ms:invalidInput", "The sampling rate must be a real scalar."));
    
    /* create matcher */
    MatchSyllables ms(sampling_rate);
    
    std::vector<float> syllable;
    for (unsigned int i = 2; i < nrhs; ++i) {
        getVector(prhs[i], syllable, "MATLAB:ms:invalidInput", "Each syllable must be a real vector.");
        if (ms.AddSyllable(syllable, 1.0) == -1) {
            mexErrMsgIdAndTxt("MATLAB:ms:invalidInput", "Unable to add syllable.");
        }
    }
    
    ms.SetCallbackColumn(cbAppendResult);
    
    // initialize
    if (!ms.Initialize()) {
        mexErrMsgIdAndTxt("MATLAB:ms:internalError", "Unable to initialize syllable matcher.");
    }
    
    // chunks
    size_t chunk_size = 1024;
    std::vector<float>::iterator begin = signal.begin();
    for (size_t i = 0; i < signal.size(); i += chunk_size) {
        if (!ms.IngestAudio(std::vector<float>(begin + i, begin + (i + chunk_size < signal.size() ? i + chunk_size : signal.size())))) {
            mexErrMsgIdAndTxt("MATLAB:ms:internalError", "Unable to ingest audio.");
        }
    }
    
    // generate outputs
    size_t rows = result_score.size(), cols = rows > 0 ? result_score[0].size() : 0;
    double *scores;
    double *lengths;
    plhs[0] = mxCreateDoubleMatrix(rows, cols, mxREAL);
    scores = mxGetPr(plhs[0]);
    plhs[1] = mxCreateDoubleMatrix(rows, cols, mxREAL);
    lengths = mxGetPr(plhs[1]);
    
    // fill outputs
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            scores[i * cols + j] = result_score[i][j];
            lengths[i * cols + j] = result_length[i][j];
        }
    }
}
