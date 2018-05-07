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

To use on macOS, you must have:

* Xcode - The project is packaged as an Xcode project for building and testing, and installing Xcode installs the necessary compiler and other functionality for compiling the mex files.


Compiling on Bela
-----------------

To install, create a feedback project on the bela. Once created, you can copy or update the needed files using the command:

```
> ./bela_update.sh
```

Alternatively, you can use the web based IDE or manually move files onto the Bela. The project directory structure does not need to be preserved, and to run, you need to upload the appropriate "render.cpp" file and all files in "BelaWarpDetect/Library".

Note that the project needs C++11. As a result, it should be configured with the make parameter `CPPFLAGS=-std=c++11`.

MATLAB Interface
----------------

To help with testing, there are MATLAB MEX implementations that allow using the matching code in MATLAB. To compile the MEX functions, switch to the "BelaWarpDetect/Matlab" folder and run:

```
>> compile_mex
````

The following functions are available:

### Match Syllables

The match syllables function looks for renditions of one or more syllables in an audio file, and will return both the scores and lengths from the dynamic time warping function. To use:

```
>> [scores, lengths] = match_syllables(audio, fs, syllable1, ...);
```

Parameter `audio` contains the audio stream within which to search for the syllables (`syllable1`, etc), while `fs` is the sampling rate for both the audio and the syllables. The function returns `scores` and `lengths` for each spectral column of `audio` (rows) and for each syllable (columns).

The syllable(s) can either be audio or can be a spectral template (see `build_template` for constructing spectral templates).

### Evaluate Syllables

Evaluates a single syllable, returning scores based on other syllable renditions.

```
>> [scores, lengths] = eval_syllable(syllable, fs, audio1, ...);
```

Parameter `syllable` contains the audio or spectral template (see `build_template` for constructing spectral templates) for a single syllable, while `fs` is the sampling rate for both the syllables and audio. Multiple short audio segments (`audio1`, etc) are past through the matcher and the score and length at the end is returned.

This helps in building confusion matrices to understand how effective the spectral template or audio is at separating syllables.
