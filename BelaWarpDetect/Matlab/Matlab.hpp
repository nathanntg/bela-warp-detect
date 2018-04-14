//
//  Matlab.hpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 4/10/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#ifndef Matlab_h
#define Matlab_h

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
    else {
        mexErrMsgIdAndTxt(err_id, err_str);
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

template <typename T> ManagedMemory<T> getMatrix(const mxArray *in, size_t &sm, size_t &sn, const char *err_id, const char *err_str) {
    size_t sl;
    
    // check matrix
    if (!testMatrix(in)) {
        mexErrMsgIdAndTxt(err_id, err_str);
    }
    
    sm = mxGetM(in);
    sn = mxGetN(in);
    sl = sn * sm;
    ManagedMemory<T> mat(sn * sm);
    
    // fill matrix
    if (mxIsDouble(in)) {
        double *values = static_cast<double *>(mxGetPr(in));
        for (size_t i = 0; i < sl; ++i) {
            mat[i] = static_cast<T>(values[i]);
        }
    }
    else if (mxIsSingle(in)) {
        float *values = reinterpret_cast<float *>(mxGetPr(in));
        for (size_t i = 0; i < sl; ++i) {
            mat[i] = static_cast<T>(values[i]);
        }
    }
    else {
        mexErrMsgIdAndTxt(err_id, err_str);
    }
    
    return mat;
}

#endif /* Matlab_h */
