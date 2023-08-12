@echo off

mkdir out
pushd out

cl^
 /Zi /MDd^
 /IC:\vclib\SDL-release-2.26.4\include^
 /IC:\vclib\SDL_image-release-2.6.3\include^
 /IC:\vclib\SDL_ttf-release-2.20.2\include^
 /IC:\vclib\SDL_mixer-release-2.6.3\include^
 ..\src\main.cpp ..\src\Game.cpp^
 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib^
 Winmm.lib Setupapi.lib Version.lib Imm32.lib^
 C:\vclib\SDL-release-2.26.4\VisualC\Win32\Debug\SDL2.lib^
 C:\vclib\SDL-release-2.26.4\VisualC\Win32\Debug\SDL2main.lib^
 C:\vclib\SDL_image-release-2.6.3\VisualC\Win32\Debug\SDL2_image.lib^
 C:\vclib\SDL_ttf-release-2.20.2\VisualC\Win32\Debug\SDL2_ttf.lib^
 C:\vclib\SDL_mixer-release-2.6.3\VisualC\Win32\Debug\SDL2_mixer.lib^
 /link^
 /SUBSYSTEM:CONSOLE

popd