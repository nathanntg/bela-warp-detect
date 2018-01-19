Bela Warp Detect
================

This project is used to do syllable detection or template matching in real time
on an audio signal. This implementation is inspired by two earlier projects:

* [Syllable Detector](https://github.com/gardner-lab/syllable-detector-swift) - A neural network based syllable detector that feeds a spectrogram into a simple network (perceptron) for matching. Achieves low-latency, high accuracy detection. Requires a MATLAB trained neural network.
* [Find Audio](https://github.com/gardner-lab/find-audio) - A MATLAB library (and accompanying MEX files) to use a variant of dynamic time warping to find and align renditions of an audio template in a longer signal.

The implementation here uses a dynamic programming approach to syllable detection, 
reducing the need to amass a huge amount of data or perform a long training process.

The detection is designed to be usable from MATLAB (for training and performance 
evaluation), and is designed to be run on a [Bela](http://bela.io) embedded platform 
(which uses a real time operating system and is capable of audio signal processing).


Requirements
------------

The code supports:

* macOS (with a few additional requirements), via either the C++ API or MATLAB MEX files
* Bela embedded platform

To use on macOS, you must install:

* [libsndfile](http://www.mega-nerd.com/libsndfile/) - The same framework used by Bela for loading audio files. (Could potentially remove this requirement by using system APIs for parsing audio.)
* Xcode - The project is packaged as an Xcode project for building and testing, and installing Xcode installs the necessary compiler and other functionality for compiling the mex files.
