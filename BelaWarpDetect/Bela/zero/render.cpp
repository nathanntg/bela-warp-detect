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
bool gOutput = false;

// anciliary task
AuxiliaryTask gMatchTask;

// loggin file
WriteFile gLogFile;

void process_match_background(void *);

bool setup(BelaContext *context, void *userData)
{
    rt_printf("Setup.\n");
    
    // create marcher
    gMatcher = new MatchSyllables(context->audioSampleRate);
    
    // load syllable
    if (-1 == gMatcher->AddSpectrogram("syllable01.bin", 1)) {
        rt_printf("Unable to load syllable file.\n");
        return false;
    }
    
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
    float scores[1];
    int lens[1];
    while (gMatcher->MatchOnce(&scores[0], &lens[0])) {
        if (scores[0] > 0.372151 && lens[0] > -14 && lens[0] < 14) {
            gLogFile.log(scores[0]);
            gOutput = true;
        }
    }
}

void render(BelaContext *context, void *userData)
{
    unsigned int numAudioFrames = context->audioFrames;
    unsigned int numAudioInChannels = context->audioInChannels;
    unsigned int numAudioOutChannels = context->audioOutChannels;
    
    // input
    if (context->flags & BELA_FLAG_INTERLEAVED) {
        // interleaved input, use stride
        gMatcher->IngestAudio(context->audioIn, numAudioFrames, numAudioInChannels);
    }
    else {
        // use first channel (not interleaved)
        gMatcher->IngestAudio(context->audioIn, numAudioFrames);
    }
    
    // output
    
    // zero outputs
    for (unsigned int i = 0; i < numAudioFrames * numAudioOutChannels; ++i) {
        context->audioOut[i] = 0.0;
    }
    
    // WHITE NOISE (test, constant white noise on right channel)
    // for (unsigned int i = 0; i < numAudioFrames; ++i) {
    //     if (context->flags & BELA_FLAG_INTERLEAVED) {
    //         context->audioOut[i * numAudioOutChannels] = 0.0;
    //         context->audioOut[i * numAudioOutChannels + 1] = 0.4f * ( rand() / (float)RAND_MAX * 2.f - 1.f);
    //     }
    //     else {
    //         context->audioOut[i] = 0.0;
    //         context->audioOut[i + numAudioFrames] = 0.4f * ( rand() / (float)RAND_MAX * 2.f - 1.f);
    //     }
    // }
    
    // add TTL pulse
    if (gOutput) {
        for (unsigned int i = 0; i < 44; ++i) {
            if (context->flags & BELA_FLAG_INTERLEAVED) {
                context->audioOut[i * numAudioOutChannels] = 1.0;
            }
            else {
                context->audioOut[i] = 1.0;
            }
        }
        gOutput = false;
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
