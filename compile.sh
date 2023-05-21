#!/usr/bin/zsh

if [ $# -lt 2 ]; then
    echo "Usage: ./compile.sh <AST file> <output bin path>"
    exit
fi

filename=$(basename $1 .edoc)

mkdir -p dump # for graphic dumps from frontend / middleend

./bin/front               $1                      /tmp/$filename.ast
./bin/middle              /tmp/$filename.ast      /tmp/$filename.opt.ast
./bin/x64_compiler        /tmp/$filename.opt.ast  $2

chmod +x $2