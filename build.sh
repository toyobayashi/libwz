#!/usr/bin/env bash

set -e

cd "$(dirname "$0")"

if [ ! -d "build" ]; then
  cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Debug -DBUILD_JNI=ON
fi

cmake --build build
