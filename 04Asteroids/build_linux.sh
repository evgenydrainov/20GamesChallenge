#!/bin/sh
mkdir out
gcc -o out/04Asteroids \
    src/main.cpp src/Game.cpp src/Font.cpp src/Sprite.cpp \
    `sdl2-config --cflags --libs` -lSDL2main -lSDL2_image -lSDL2_ttf -lSDL2_mixer
# ./build.sh && out/04Asteroids