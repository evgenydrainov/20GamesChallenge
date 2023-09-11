@echo off
mkdir out
g++ -o out/main.exe src/*.cpp^
 -IC:\vclib\SDL2-2.28.3\i686-w64-mingw32\include\SDL2^
 -IC:\vclib\SDL2_image-2.6.3\i686-w64-mingw32\include\SDL2^
 -IC:\vclib\SDL2_ttf-2.20.2\i686-w64-mingw32\include\SDL2^
 -IC:\vclib\SDL2_mixer-2.6.3\i686-w64-mingw32\include\SDL2^
 -DFIBER_FLAG_FLOAT_SWITCH=1^
 -LC:\vclib\SDL2-2.28.3\i686-w64-mingw32\lib^
 -LC:\vclib\SDL2_image-2.6.3\i686-w64-mingw32\lib^
 -LC:\vclib\SDL2_ttf-2.20.2\i686-w64-mingw32\lib^
 -LC:\vclib\SDL2_mixer-2.6.3\i686-w64-mingw32\lib^
 -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer