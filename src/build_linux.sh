#!/bin/bash

if [ "$1" = "gcc" ]; then
    gcc *.c -lm -lX11 -lGL -lXi -D X11 -o glsl-viewer
elif [ "$1" = "tcc" ]; then
    tcc *.c -lm -lX11 -lGL -lXi -D X11 -o glsl-viewer
else
    echo "usage: build_linux.sh [gcc | tcc]"
fi
