//
//  ManagedMemory.hpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/23/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#ifndef ManagedMemory_hpp
#define ManagedMemory_hpp

#include <stdio.h>
#include <stdexcept>

template <typename T>
class ManagedMemory
{
public:
    // constructor
    ManagedMemory(const size_t size) : _size(size), _ptr(new T[size]()) { }
    
    // destructor
    ~ManagedMemory() { if (_ptr != nullptr) { delete[] _ptr; } }
    
    // copy
    ManagedMemory(const ManagedMemory<T> &rhs) : _size(rhs._size), _ptr(new T[rhs._size]) {
        std::copy(rhs._ptr, rhs._ptr + _size, _ptr);
    }
    
    // move
    ManagedMemory(ManagedMemory<T> &&rhs) : _size(rhs._size), _ptr(nullptr) {
        // copy pointer
        _ptr = rhs._ptr;
        
        // remove the other pointer, to prevent releasing the memory
        rhs._ptr = nullptr;
    }
    
    // copy assignment
    ManagedMemory<T> &operator=(const ManagedMemory<T> &rhs) {
        // must match size
        if (_size != rhs._size) {
            throw std::invalid_argument("sizes must match");
        }
        
        if (this != &rhs) {
            // free existing
            delete[] _ptr;
            
            // allocate new and cpoy
            _ptr = new T[_size];
            std::copy(rhs._ptr, rhs._ptr + _size, _ptr);
        }
        
        return *this;
    }
    
    // move assignment
    ManagedMemory<T> &operator=(ManagedMemory<T>&& rhs) {
        // must match size
        if (_size != rhs._size) {
            throw std::invalid_argument("sizes must match");
        }
        
        if (this != &rhs) {
            // free existing
            delete[] _ptr;
            
            // copy pointer
            _ptr = rhs._ptr;
            
            // remove the other pointer, to prevent releasing the memory
            rhs._ptr = nullptr;
        }
        
        return *this;
    }
    
    // size
    size_t size() const { return _size; }
    
    // pointer
    T *ptr() const { return _ptr; }
    const T * const ptr_const() const { return _ptr; }
    
    // subscript
    //const T& operator[] (const size_t index) { return _ptr[index]; } // read only
    T& operator[] (const size_t index) { // add const at beginning to make read only
        if (index >= _size) {
            throw std::out_of_range("index must be less than size");
        }
        
        return _ptr[index];
    }
    
private:
    const size_t _size;
    T *_ptr;
};

#endif /* ManagedMemory_hpp */
