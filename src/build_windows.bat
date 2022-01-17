@echo off
if [%1]==[] goto usage
if %1==gcc goto gcc
if %1==tcc goto tcc
if %1==msvc goto msvc
if %1==clang goto clang

:gcc
gcc.exe *.c -Ofast -lopengl32 -luser32 -lgdi32 -o glsl-viewer.exe

:tcc
tcc.exe *.c -lgdi32 -luser32 -lopengl32 -o glsl-viewer.exe

:msvc
if not defined DevEnvDir (
    call vcvarsall.bat x64
)
cl *.c /Ox user32.lib gdi32.lib Opengl32.lib /Feglsl-viewer.exe
del *.obj

:clang
clang.exe *.c -lgdi32 -luser32 -lopengl32 -o glsl-viewer.exe

goto end
:usage
echo usage: build_windows.bat [gcc ^| tcc ^| msvc ^| clang]
:end