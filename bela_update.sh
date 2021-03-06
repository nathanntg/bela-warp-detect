#!/usr/bin/env bash

# set -o pipefail # get errors even when piped
# set -o xtrace # debug
set -o nounset # prevent using undeclared variables
set -o errexit # exit on command fail; allow failure: || true

# copy files
scp BelaWarpDetect/Library/* root@192.168.7.2:~/Bela/projects/feedback
scp BelaWarpDetect/Bela/zero/render.cpp root@192.168.7.2:~/Bela/projects/feedback
