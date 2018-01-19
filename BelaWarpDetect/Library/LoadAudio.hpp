//
//  LoadAudio.hpp
//  BelaWarpDetect
//
//  Created by Nathan Perkins on 1/19/18.
//  Copyright © 2018 Nathan Perkins. All rights reserved.
//

#ifndef LoadAudio_hpp
#define LoadAudio_hpp

#include <stdio.h>
#include <string>
#include <vector>

bool LoadAudio(std::string file, std::vector<float> &samples, float &sample_rate, unsigned int channel = 0);

#endif /* LoadAudio_hpp */
