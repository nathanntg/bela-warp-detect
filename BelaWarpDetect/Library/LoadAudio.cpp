//
//  LoadAudio.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/19/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include "LoadAudio.hpp"

#if defined(__APPLE__)
#include <iostream>

#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreFoundation/CoreFoundation.h>

// TODO: write using core audio
// https://developer.apple.com/library/content/technotes/tn2097/_index.html#//apple_ref/doc/uid/DTS10003287-CH1-PARTTWO

struct readData {
    void *fileBuffer;
    void *sourceBuffer;
    UInt64 packetOffset;
    UInt64 packetCount;
    UInt32 maxPacketSize;
    UInt64 byteOffset;
};

OSStatus complexInputCallback(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData) {
    OSStatus ret = noErr;
    UInt32 bytesCopied = 0;
    
    // data
    struct readData *info = static_cast<struct readData *>(inUserData);
    
    // initialize in case of failure
    ioData->mBuffers[0].mData = NULL;
    ioData->mBuffers[0].mDataByteSize = 0;
    
    // if there are not enough packets, read what is left
    if (info->packetOffset + *ioNumberDataPackets > info->packetCount) {
        *ioNumberDataPackets = info->packetCount - info->packetOffset;
    }
    
    // number of packets
    if (*ioNumberDataPackets) {
        if (NULL != info->sourceBuffer) {
            free(info->sourceBuffer);
            info->sourceBuffer = NULL;
        }
        
        // total bytes requested
        bytesCopied = *ioNumberDataPackets * info->maxPacketSize;
        info->sourceBuffer = static_cast<UInt8 *>(calloc(bytesCopied, 1));
        
        // copy data
        memcpy(info->sourceBuffer, static_cast<UInt8 *>(info->fileBuffer) + info->byteOffset, bytesCopied);
        
        // update offsets
        info->packetOffset += *ioNumberDataPackets;
        info->byteOffset += *ioNumberDataPackets * info->maxPacketSize;
        
        // pass pointer
        ioData->mBuffers[0].mData = info->sourceBuffer;
        ioData->mBuffers[0].mDataByteSize = bytesCopied;
    }
    
    return ret;
}

bool LoadAudio(std::string file, std::vector<float> &samples, float &sample_rate, unsigned int channel) {
    AudioFileID fileID;
    AudioStreamBasicDescription formatInput;
    
    // create file reference
    CFURLRef fileRef = CFURLCreateWithBytes(NULL, reinterpret_cast<const UInt8 *>(file.data()), file.size(), kCFStringEncodingUTF8, NULL);
    
    // open file
    if (noErr != AudioFileOpenURL(fileRef, kAudioFileReadPermission, 0, &fileID)) {
        CFRelease(fileRef);
        return false;
    }
    
    // reusable size variable
    UInt32 size;
    
    // get file data
    size = sizeof(formatInput);
    if (noErr != AudioFileGetProperty(fileID, kAudioFilePropertyDataFormat, &size, &formatInput)) {
        AudioFileClose(fileID);
        CFRelease(fileRef);
        return false;
    }
    
    // load other useful data
    UInt64 countPackets, countBytes;
    UInt32 packetMaxSize;
    
    // get packet count
    size = sizeof(countPackets);
    if (noErr != AudioFileGetProperty(fileID, kAudioFilePropertyAudioDataPacketCount, &size, &countPackets)) {
        AudioFileClose(fileID);
        CFRelease(fileRef);
        return false;
    }
    
    // get packet count
    size = sizeof(countBytes);
    if (noErr != AudioFileGetProperty(fileID, kAudioFilePropertyAudioDataByteCount, &size, &countBytes)) {
        AudioFileClose(fileID);
        CFRelease(fileRef);
        return false;
    }
    
    // get packet count
    size = sizeof(packetMaxSize);
    if (noErr != AudioFileGetProperty(fileID, kAudioFilePropertyMaximumPacketSize, &size, &packetMaxSize)) {
        AudioFileClose(fileID);
        CFRelease(fileRef);
        return false;
    }
    
    // configure output format
    AudioStreamBasicDescription formatOutput;
    formatOutput.mFormatID = kAudioFormatLinearPCM;
    formatOutput.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    formatOutput.mSampleRate = formatInput.mSampleRate;
    formatOutput.mFramesPerPacket = 1;
    formatOutput.mBytesPerPacket = formatInput.mChannelsPerFrame * sizeof(float);
    formatOutput.mBytesPerFrame = formatInput.mChannelsPerFrame * sizeof(float);
    formatOutput.mChannelsPerFrame = formatInput.mChannelsPerFrame;
    formatOutput.mBitsPerChannel = 8 * sizeof(float);
    
    // invalid channel?
    if (channel >= formatOutput.mChannelsPerFrame) {
        AudioFileClose(fileID);
        CFRelease(fileRef);
        return false;
    }
    
    // create converter
    AudioConverterRef converter;
    if (noErr != AudioConverterNew(&formatInput, &formatOutput, &converter)) {
        AudioFileClose(fileID);
        CFRelease(fileRef);
        return false;
    }
    
    // read file into memory
    UInt32 bytes = static_cast<UInt32>(countBytes);
    UInt32 packets = static_cast<UInt32>(countPackets);
    void *fileBuffer = malloc(countBytes);
    
    // read
    if (noErr != AudioFileReadPacketData(fileID, false, &bytes, NULL, 0, &packets, fileBuffer)) {
        free(fileBuffer);
        AudioConverterDispose(converter);
        AudioFileClose(fileID);
        CFRelease(fileRef);
        return false;
    }
    
    // output buffers
    UInt32 outputBufferSize = 32768;
    void *outputBuffer = malloc(outputBufferSize);
    UInt32 outputBufferPackets = outputBufferSize / formatOutput.mBytesPerPacket;
    
    // convert
    struct readData info = { .fileBuffer = fileBuffer, .sourceBuffer = NULL, .packetOffset = 0, .packetCount = countPackets, .maxPacketSize = packetMaxSize, .byteOffset = 0 };
    
    // clear samples
    samples.clear();
    sample_rate = formatOutput.mSampleRate;
    
    while (true) {
        // output holder
        AudioBufferList out = {};
        out.mNumberBuffers = 1;
        out.mBuffers[0].mNumberChannels = formatOutput.mChannelsPerFrame;
        out.mBuffers[0].mDataByteSize = outputBufferSize;
        out.mBuffers[0].mData = outputBuffer;
        
        // convert
        UInt32 ioOutputDataPackets = outputBufferPackets;
        if (noErr != AudioConverterFillComplexBuffer(converter, complexInputCallback, static_cast<void *>(&info), &ioOutputDataPackets, &out, NULL)) {
            free(fileBuffer);
            AudioConverterDispose(converter);
            AudioFileClose(fileID);
            CFRelease(fileRef);
            return false;
        }
        
        // success
        if (ioOutputDataPackets == 0) {
            break;
        }
        
        float *ptr = static_cast<float *>(out.mBuffers[0].mData);
        for (UInt32 i = 0; i < ioOutputDataPackets; ++i) {
            samples.push_back(ptr[channel + i * formatOutput.mChannelsPerFrame]);
        }
    }
    
    // cleanup
    free(fileBuffer);
    AudioConverterDispose(converter);
    AudioFileClose(fileID);
    CFRelease(fileRef);
    
    return true;
}

#else
#include <sndfile.h>

bool LoadAudio(std::string file, std::vector<float> &samples, float &sample_rate, unsigned int channel) {

    SNDFILE *sf;
    SF_INFO sf_info;
    sf_info.format = 0;
    
    // open file
    if (!(sf = sf_open(file.c_str(), SFM_READ, &sf_info))) {
        return false;
    }
    
    // unexpected number of channels
    if (channel >= sf_info.channels) {
        return false;
    }
    
    // sample rate
    sample_rate = static_cast<float>(sf_info.samplerate);
    
    // prepare memory
    float *mem = static_cast<float *>(malloc(sizeof(float) * sf_info.channels * sf_info.frames));
    
    // seek start frame
    sf_seek(sf, 0, SEEK_SET);
    
    // read
    sf_count_t read_count = sf_read_float(sf, mem, sf_info.channels * sf_info.frames);
    
    // fail if did not receive expected number of samples
    if (read_count != (sf_info.channels * sf_info.frames)) {
        sf_close(sf);
        free(mem);
        return false;
    }
    
    // zero pad, in case file reading failed (maybe return false?)
//    for (unsigned int i = read_count; i < sf_info.frames * sf_info.channels; ++i) {
//        samples[i] = 0.0;
//    }
    
    // scale
    float scale = 1.0;
    
    // re-scale floating point numbers
//    int subformat = sf_info.format & SF_FORMAT_SUBMASK;
//    if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE) {
//        // fetch maximum
//        double t;
//        sf_command(sf, SFC_CALC_SIGNAL_MAX, &t, sizeof(t));
//
//        // if non-zero, use as scaling factor
//        if (t > 1e-10) {
//            // the example code uses 32700.0 as a scaling factor... WHY?
//            scale = 1.0 / t;
//        }
//    }
    
    // resize samples
    samples.resize(sf_info.frames);
    for (unsigned int i = 0; i < sf_info.frames; ++i) {
        samples[i] = mem[i * sf_info.channels + channel] * scale;
    }
    
    // clean up
    sf_close(sf);
    free(mem);
    
    return true;
}
#endif
