#!/bin/sh
gcc -o out/04Asteroids src/main.cpp src/Game.cpp `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer
# ./build.sh && out/04Asteroids