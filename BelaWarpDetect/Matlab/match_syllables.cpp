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
#include "Matlab/Matlab.hpp"

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
        if (testVector(prhs[i])) {
            getVector(prhs[i], syllable, "MATLAB:ms:invalidInput", "The syllable must be a real vector or matrix.");
            if (ms.AddSyllable(syllable, 1e6) == -1) {
                mexErrMsgIdAndTxt("MATLAB:ms:internalError", "Unable to add the syllable to the matcher.");
            }
        }
        else if (testMatrix(prhs[i])) {
            size_t sn, sm;
            ManagedMemory<float> mat(getMatrix<float>(prhs[i], sm, sn, "MATLAB:ms:invalidInput", "The syllable must be a real vector or matrix."));
            if (ms.AddSpectrogram(mat.ptr(), sn, sm, 1e6) == -1) {
                mexErrMsgIdAndTxt("MATLAB:ms:internalError", "Unable to add the syllable to the matcher.");
            }
        }
        else {
            mexErrMsgIdAndTxt("MATLAB:ms:invalidInput", "The syllable must be a real vector or matrix.");
        }
    }
    
    // clear outputs
    result_score.clear();
    result_length.clear();
    
    ms.SetCallbackColumn(cbAppendResult);
    
    // initialize
    if (!ms.Initialize()) {
        mexErrMsgIdAndTxt("MATLAB:ms:internalError", "Unable to initialize syllable matcher.");
    }
    
    // chunks
    size_t chunk_size = 1024;
    std::vector<float>::iterator begin = signal.begin();
    for (size_t i = 0; i < signal.size(); i += chunk_size) {
        // ingest audio
        if (!ms.IngestAudio(std::vector<float>(begin + i, begin + (i + chunk_size < signal.size() ? i + chunk_size : signal.size())))) {
            mexErrMsgIdAndTxt("MATLAB:ms:internalError", "Unable to ingest audio.");
        }
        
        // perform matching
        ms.PerformMatching();
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
            scores[i + j * rows] = result_score[i][j];
            lengths[i + j * rows] = result_length[i][j];
        }
    }
    
    // clear outputs
    result_score.clear();
    result_length.clear();
}
