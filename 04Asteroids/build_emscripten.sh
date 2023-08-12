#!/bin/sh
emcc src/main.cpp src/Game.cpp -s WASM=1 -s ASYNCIFY=1 \
    -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s USE_SDL_TTF=2 -s USE_SDL_MIXER=2 \
    --preload-file assets -o out/index.html
# emrun out/index.html