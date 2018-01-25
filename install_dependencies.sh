#!/usr/bin/env bash

# set -o pipefail # get errors even when piped
# set -o xtrace # debug
set -o nounset # prevent using undeclared variables
set -o errexit # exit on command fail; allow failure: || true

if [ ! -d "TestBelaWarpDetect" ]; then
	>&2 echo "Unable to find expected repository directories."
	exit 1
fi

if [ ! -f "TestBelaWarpDetect/catch.hpp" ]; then
	echo "Downloading header version of catch.hpp..."
	curl -L -o TestBelaWarpDetect/catch.hpp https://github.com/catchorg/Catch2/releases/download/v2.1.0/catch.hpp
fi


