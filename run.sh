#!/usr/bin/zsh

./asm data/test.asm data/test.bin

cd cmake-build-debug || exit
cmake ..
ninja all

cd ..
edb --run cmake-build-debug/x64_translator