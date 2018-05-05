//
//  main.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/11/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <atomic>
#include <iostream>
#include <csignal>
#include <vector>
#include <ctime>

#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/CoreAudio.h>
#include <dispatch/dispatch.h>

#include "MatchSyllables.hpp"

// should keep running (listens for interrupt)
bool gRun = true;

// output status
std::atomic<unsigned int> gTTL(0);
std::atomic<unsigned int> gFeedbackSamples(0);
//unsigned int gFeedbackDebounce = 0;

// dispatch queue
// TODO: move into class strucutre, release (dispatch_release)
dispatch_queue_t g_queue = dispatch_queue_create("ProcessorQueue", DISPATCH_QUEUE_SERIAL);

// TODO: move into class structure
AudioComponentInstance outputUnit;
AudioComponentInstance inputUnit;
AudioBufferList *inputBufferList;

OSStatus on_input(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    OSStatus ret = noErr;
    
    MatchSyllables *matcher = static_cast<MatchSyllables *>(inRefCon);
    
    ret = AudioUnitRender(inputUnit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, inputBufferList);
    
    if (noErr == ret) {
        float *audio = static_cast<float *>(inputBufferList->mBuffers[0].mData);
        unsigned int length = static_cast<unsigned int>(inNumberFrames);
        
        // match
        if (!matcher->IngestAudio(audio, length)) {
            // memory error: AVErrorOutOfMemory
            return -11801;
        }
        
        // dispatch
        // https://developer.apple.com/documentation/dispatch/1453057-dispatch_async?language=objc
        dispatch_async(g_queue, ^{
            matcher->PerformMatching();
        });
    }
    
    return noErr;
}

OSStatus on_output(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
//
//    // for each channel
//    for (unsigned int i = 0; i < ioData->mNumberBuffers; ++i) {
//        // for each sample
//        for (unsigned int )
//
//    }
    
    unsigned int i, j;
    float *output;
    
    // zero output
    for (i = 0; i < ioData->mNumberBuffers; ++i) {
        output = static_cast<float *>(ioData->mBuffers[i].mData);
        for (j = 0; j < inNumberFrames; ++j) {
            output[j] = 0;
        }
    }
    
    switch (gTTL) {
        case 1:
            // clear TTL
            gTTL = 0;
            
            // positive pulse
            output = static_cast<float *>(ioData->mBuffers[0].mData);
            for (i = 0; i < inNumberFrames; ++i) {
                output[i] = 1.0;
            }
            break;
            
        case 2:
            // clear TTL
            gTTL = 0;
            
            // negative pulse
            output = static_cast<float *>(ioData->mBuffers[0].mData);
            for (i = 0; i < inNumberFrames; ++i) {
                output[i] = -1.0;
            }
            break;
    }
    
    if (0 < gFeedbackSamples && ioData->mNumberBuffers >= 2) {
        unsigned int desired_samples = gFeedbackSamples;
        unsigned int samples = MIN(inNumberFrames, desired_samples);
        gFeedbackSamples = desired_samples - samples;
        
        // output
        output = static_cast<float *>(ioData->mBuffers[1].mData);
        for (unsigned int i = 0; i < samples; ++i) {
            output[i] = 0.4f * ( rand() / (float)RAND_MAX * 2.f - 1.f);
        }
    }
    
    return noErr;
}

void on_signal(int signum) {
    std::cout << "Interrupt signal: " << signum << std::endl;
    gRun = false;
}

void on_match(size_t index, float score, int length) {
    if (0 == index) {
        // ttl pulse
        gTTL = 1;
        
        // 30ms of feedback
        gFeedbackSamples = 1323;
    }
    
    // current time
    time_t now = time(0);
    char *dt = ctime(&now); // includes a new line character
    
    // log it
    std::cout << index << "\t" << score << "\t" << length << "\t" << dt;
}

AudioDeviceID defaultDeviceInput() {
    AudioDeviceID device;
    UInt32 size = sizeof(device);
    
    AudioObjectPropertyAddress address = { .mSelector = kAudioHardwarePropertyDefaultInputDevice, .mScope = kAudioObjectPropertyScopeGlobal, .mElement = kAudioObjectPropertyElementMaster };
    
    if (noErr != AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, NULL, &size, &device)) {
        std::cerr << "Unable to get default input device." << std::endl;
        exit(1);
    }
    
    return device;
}

AudioDeviceID defaultDeviceOutput() {
    AudioDeviceID device;
    UInt32 size = sizeof(device);
    
    AudioObjectPropertyAddress address = { .mSelector = kAudioHardwarePropertyDefaultOutputDevice, .mScope = kAudioObjectPropertyScopeGlobal, .mElement = kAudioObjectPropertyElementMaster };
    
    if (noErr != AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, NULL, &size, &device)) {
        std::cerr << "Unable to get default output device." << std::endl;
        exit(1);
    }
    
    return device;
}

int main(int argc, const char * argv[]) {
    // frame size
    UInt32 frameSize = 32;
    
    // reusable size holder
    UInt32 size;
    
    // CREATE OUTPUT AUDIO
    
    // set input bus
    AudioUnitElement outputBus = 0;
    
    // describe component
    AudioComponentDescription outputComponentDescription = { .componentType = kAudioUnitType_Output, .componentSubType = kAudioUnitSubType_DefaultOutput, .componentManufacturer = kAudioUnitManufacturer_Apple, .componentFlags = 0, .componentFlagsMask = 0 };
    
    // find next component
    AudioComponent outputComponent = AudioComponentFindNext(NULL, &outputComponentDescription);
    if (!outputComponent) {
        std::cerr << "Unable to find output component." << std::endl;
        return 1;
    }
    
    // make audio unit
    if (noErr != AudioComponentInstanceNew(outputComponent, &outputUnit)) {
        std::cerr << "Unable to initialize output unit." << std::endl;
        return 1;
    }
    
    // set output device
    AudioDeviceID outputDevice = defaultDeviceOutput();
    if (noErr != AudioUnitSetProperty(outputUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &outputDevice, sizeof(outputDevice))) {
        std::cerr << "Unable to specify output device." << std::endl;
        return 1;
    }
    
    // get the audio format
    AudioStreamBasicDescription outputOutputFormat, outputInputFormat;
    size = sizeof(outputOutputFormat);
    if (noErr != AudioUnitGetProperty(outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, outputBus, &outputOutputFormat, &size)) {
        std::cerr << "Unable to fetch output format." << std::endl;
        return 1;
    }
    
    // check for expected format
    if (kAudioFormatLinearPCM != outputOutputFormat.mFormatID || 1 != outputOutputFormat.mFramesPerPacket || kAudioFormatFlagsNativeFloatPacked != outputOutputFormat.mFormatFlags) {
        std::cerr << "Unsupported audio format." << std::endl;
        return 1;
    }
    
    // configure output format
    outputInputFormat.mFormatID = kAudioFormatLinearPCM;
    outputInputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved;
    outputInputFormat.mSampleRate = outputOutputFormat.mSampleRate;
    outputInputFormat.mFramesPerPacket = 1;
    outputInputFormat.mBytesPerPacket = sizeof(float);
    outputInputFormat.mBytesPerFrame = sizeof(float);
    outputInputFormat.mChannelsPerFrame = outputOutputFormat.mChannelsPerFrame;
    outputInputFormat.mBitsPerChannel = 8 * sizeof(float);
    
    // set the format
    if (noErr != AudioUnitSetProperty(outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, outputBus, &outputInputFormat, sizeof(outputInputFormat))) {
        std::cerr << "Unable to set output format." << std::endl;
        return 1;
    }
    
    // set the frame size
    UInt32 outputFrameSize = frameSize;
    if (noErr != AudioUnitSetProperty(outputUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &outputFrameSize, sizeof(outputFrameSize))) {
        std::cerr << "Unable to set the output frame size." << std::endl;
        return 1;
    }
    
    // set callback
    AURenderCallbackStruct outputCallbackStruct = { .inputProc = on_output, .inputProcRefCon = NULL };
    if (noErr != AudioUnitSetProperty(outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, 0, &outputCallbackStruct, sizeof(outputCallbackStruct))) {
        std::cerr << "Unable to set the output callback." << std::endl;
        return 1;
    }
    
    // CREATE INPUT AUDIO
    
    // set input bus
    AudioUnitElement inputBus = 1;
    
    // describe component
    AudioComponentDescription inputComponentDescription = { .componentType = kAudioUnitType_Output, .componentSubType = kAudioUnitSubType_HALOutput, .componentManufacturer = kAudioUnitManufacturer_Apple, .componentFlags = 0, .componentFlagsMask = 0 };
    
    // find next component
    AudioComponent inputComponent = AudioComponentFindNext(NULL, &inputComponentDescription);
    if (!inputComponent) {
        std::cerr << "Unable to find input component." << std::endl;
        return 1;
    }
    
    // make audio unit
    if (noErr != AudioComponentInstanceNew(inputComponent, &inputUnit)) {
        std::cerr << "Unable to initialize input unit." << std::endl;
        return 1;
    }
    
    // enable input
    UInt32 enableIO = 1;
    if (noErr != AudioUnitSetProperty(inputUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, inputBus, &enableIO, sizeof(enableIO))) {
        std::cerr << "Unable to enable input." << std::endl;
        return 1;
    }
    
    // disable output
    enableIO = 0;
    if (noErr != AudioUnitSetProperty(inputUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &enableIO, sizeof(enableIO))) {
        std::cerr << "Unable to enable input." << std::endl;
        return 1;
    }
    
    // set input device
    AudioDeviceID inputDevice = defaultDeviceInput();
    if (noErr != AudioUnitSetProperty(inputUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &inputDevice, sizeof(inputDevice))) {
        std::cerr << "Unable to specify input device." << std::endl;
        return 1;
    }
    
    // get the audio format
    AudioStreamBasicDescription inputInputFormat, inputOutputFormat;
    size = sizeof(inputInputFormat);
    if (noErr != AudioUnitGetProperty(inputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, inputBus, &inputInputFormat, &size)) {
        std::cerr << "Unable to fetch input format." << std::endl;
        return 1;
    }
    
    // check for expected format
    if (kAudioFormatLinearPCM != inputInputFormat.mFormatID || 1 != inputInputFormat.mFramesPerPacket || kAudioFormatFlagsNativeFloatPacked != inputInputFormat.mFormatFlags) {
        std::cerr << "Unsupported audio format." << std::endl;
        return 1;
    }
    
    // configure output format
    inputOutputFormat.mFormatID = kAudioFormatLinearPCM;
    inputOutputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved;
    inputOutputFormat.mSampleRate = inputInputFormat.mSampleRate;
    inputOutputFormat.mFramesPerPacket = 1;
    inputOutputFormat.mBytesPerPacket = sizeof(float);
    inputOutputFormat.mBytesPerFrame = sizeof(float);
    inputOutputFormat.mChannelsPerFrame = inputInputFormat.mChannelsPerFrame;
    inputOutputFormat.mBitsPerChannel = 8 * sizeof(float);
    
    // set the format
    if (noErr != AudioUnitSetProperty(inputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, inputBus, &inputOutputFormat, sizeof(inputOutputFormat))) {
        std::cerr << "Unable to set input format." << std::endl;
        return 1;
    }
    
    // get the maximum frame size
    UInt32 inputMaxFrameSize = 0;
    size = sizeof(inputMaxFrameSize);
    if (noErr != AudioUnitGetProperty(inputUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &inputMaxFrameSize, &size)) {
        std::cerr << "Unable to get maximum frame size" << std::endl;
        return 1;
    }
    
    // allocate buffers
    inputBufferList = static_cast<AudioBufferList *>(malloc(sizeof(AudioBufferList) + (inputOutputFormat.mChannelsPerFrame - 1) * sizeof(AudioBuffer)));
    inputBufferList->mNumberBuffers = inputOutputFormat.mChannelsPerFrame;
    for (UInt32 i = 0; i < inputOutputFormat.mChannelsPerFrame; ++i) {
        inputBufferList->mBuffers[i].mDataByteSize = inputOutputFormat.mBytesPerFrame * inputMaxFrameSize;
        inputBufferList->mBuffers[i].mNumberChannels = 1; // since non-interleaved
        inputBufferList->mBuffers[i].mData = malloc(inputBufferList->mBuffers[i].mDataByteSize);
    }
    
    // set the frame size
    UInt32 inputFrameSize = frameSize;
    if (noErr != AudioUnitSetProperty(inputUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &inputFrameSize, sizeof(inputFrameSize))) {
        std::cerr << "Unable to set the frame size." << std::endl;
        return 1;
    }
    
    // create marcher
    MatchSyllables matcher(inputInputFormat.mSampleRate);
    
    // set callback
    AURenderCallbackStruct inputCallbackStruct = { .inputProc = on_input, .inputProcRefCon = static_cast<void *>(&matcher) };
    if (noErr != AudioUnitSetProperty(inputUnit, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 0, &inputCallbackStruct, sizeof(inputCallbackStruct))) {
        std::cerr << "Unable to set the input callback." << std::endl;
        return 1;
    }
    
    // load syllable
    if (-1 == matcher.AddSpectrogram("syllable222.bin", 0.577435, 0.2)) {
        std::cerr << "Unable to load syllable file." << std::endl;
        return 1;
    }
    
    // set callback
    matcher.SetCallbackMatch(on_match);
    
    // initialize matcher
    if (!matcher.Initialize()) {
        std::cerr << "Unable to initialize matcher." << std::endl;
        return 1;
    }
    
    // start output
    AudioUnitInitialize(outputUnit);
    AudioOutputUnitStart(outputUnit);
    
    // start input
    AudioUnitInitialize(inputUnit);
    AudioOutputUnitStart(inputUnit);
    
    // register signal handler
    signal(SIGINT, on_signal);
    
    // run
    while (gRun) {
        sleep(1);
    }
    
    // stop input
    AudioOutputUnitStop(inputUnit);
    AudioUnitUninitialize(inputUnit);
    
    // stop output
    AudioOutputUnitStop(outputUnit);
    AudioUnitUninitialize(outputUnit);
    
    // dispose of input
    AudioComponentInstanceDispose(inputUnit);
    
    // dispose of output
    AudioComponentInstanceDispose(outputUnit);
    
    // release queue
    dispatch_release(g_queue);
    
    // free buffers
    for (UInt32 i = 0; i < inputOutputFormat.mChannelsPerFrame; ++i) {
        free(inputBufferList->mBuffers[i].mData);
    }
    free(inputBufferList);
    
    return 0;
}
