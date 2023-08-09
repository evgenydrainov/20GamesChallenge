#!/bin/sh
gcc -o out/04Asteroids src/main.cpp `sdl2-config --cflags --libs` -lSDL2_image
# ./build.sh && out/04Asteroids