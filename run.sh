#!/usr/bin/zsh

set -Eeuf -o pipefail

## Compile asm stdlib
cd src/asm_stdlib
nasm -f elf64 stdlib.nasm
ld -s -S stdlib.o -o stdlib.out

## Compile C code
cd ../..
mkdir -p build
cd build
cmake -GNinja ..
ninja all
cd ..

# Compile my own code

filename=$(basename $1 .edoc)

mkdir -p dump # for graphic dumps from frontend / middleend

./bin/front               $1                      /tmp/$filename.ast
./bin/middle              /tmp/$filename.ast      /tmp/$filename.opt.ast
./build/x64_compiler      /tmp/$filename.opt.ast  $2

chmod +x $2