//
//  render.cpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/12/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#include <Bela.h>
#include <WriteFile.h>

#include "MatchSyllables.hpp"

// matcher
MatchSyllables *gMatcher;

// output status
unsigned int gTTL = 0;
unsigned int gFeedbackSamples = 0;
//unsigned int gFeedbackDebounce = 0;

// anciliary task
AuxiliaryTask gMatchTask;

// loggin file
WriteFile gLogFile;

void process_match_background(void *);
void on_match(size_t, float, int);

bool setup(BelaContext *context, void *userData)
{
    rt_printf("Setup.\n");
    
    // create marcher
    gMatcher = new MatchSyllables(context->audioSampleRate);
    
    // load syllable
    // gMatcher->AddSpectrogram("syllable04.bin", 0.392858, 0.2)
    if (-1 == gMatcher->AddSpectrogram("syllable01.bin", 0.372151, 0.2)) {
        rt_printf("Unable to load syllable file.\n");
        return false;
    }
    
    // set callback
    gMatcher->SetCallbackMatch(on_match);
    
    // initialize matcher
    if (!gMatcher->Initialize()) {
        rt_printf("Unable to initialize matcher.\n");
        return false;
    }
    
    // initialize auxiliary task
    gMatchTask = Bela_createAuxiliaryTask(&process_match_background, 90, "match");
    if (0 == gMatchTask) {
        rt_printf("Unable to create auxiliary task.\n");
        return false;
    }
    
    // create log file
    gLogFile.init("log.txt");
    gLogFile.setFormat("%.4f\n");
    gLogFile.setFileType(kText);
    gLogFile.setEchoInterval(1);
    
    return true;
}

void process_match_background(void *)
{
    gMatcher->PerformMatching();
}

void on_match(size_t index, float score, int last_len)
{
    if (0 == index) {
        // output score
        gLogFile.log(score);
        
        // ttl pulse
        gTTL = 1;
        
        // 30ms of feedback
        gFeedbackSamples = 1323;
    }
}

void render(BelaContext *context, void *userData)
{
    bool isInterleaved = context->flags & BELA_FLAG_INTERLEAVED;
    
    unsigned int numAudioFrames = context->audioFrames;
    unsigned int numAudioInChannels = context->audioInChannels;
    unsigned int numAudioOutChannels = context->audioOutChannels;
    
    // input
    if (isInterleaved) {
        // interleaved input, use stride
        gMatcher->IngestAudio(context->audioIn, numAudioFrames, numAudioInChannels);
    }
    else {
        // use first channel (not interleaved)
        gMatcher->IngestAudio(context->audioIn, numAudioFrames);
    }
    
    // zero outputs
    for (unsigned int i = 0; i < numAudioFrames * numAudioOutChannels; ++i) {
        context->audioOut[i] = 0.0;
    }
    
    // output: white noise
    if (0 < gFeedbackSamples) {
        for (unsigned int i = 0; i < numAudioFrames; ++i) {
            // generate white noise
            if (isInterleaved) {
                context->audioOut[i * numAudioOutChannels + 1] = 0.4f * ( rand() / (float)RAND_MAX * 2.f - 1.f);
            }
            else {
                context->audioOut[numAudioFrames + i] = 0.4f * ( rand() / (float)RAND_MAX * 2.f - 1.f);
            }
            
            // stop samples
            if (--gFeedbackSamples == 0) {
                break;
            }
        }
    }
    
    // output TTL
    switch (gTTL) {
        case 1:
            // positive pulse
            for (unsigned int i = 0; i < numAudioFrames; ++i) {
                // generate white noise
                if (isInterleaved) {
                    context->audioOut[i * numAudioOutChannels] = 1.0;
                }
                else {
                    context->audioOut[i] = 1.0;
                }
            }
            
            // clear TTL
            gTTL = 0;
            
            break;
            
        case 2:
            // negative pulse
            for (unsigned int i = 0; i < numAudioFrames; ++i) {
                // generate white noise
                if (isInterleaved) {
                    context->audioOut[i * numAudioOutChannels] = -1.0;
                }
                else {
                    context->audioOut[i] = -1.0;
                }
            }
            
            // clear TTL
            gTTL = 0;
            
            break;
    }
    
    // schedule task
    Bela_scheduleAuxiliaryTask(gMatchTask);
}

void cleanup(BelaContext *context, void *userData)
{
    // release matcher
    delete gMatcher;
    gMatcher = NULL;
}
