@echo off
setlocal

if /I "%1"=="" goto DEBUG
if /I "%1"=="--debug" goto DEBUG
if /I "%1"=="/debug" goto DEBUG

if /I "%1"=="--release" goto RELEASE
if /I "%1"=="/release" goto RELEASE

:DEBUG
set compilerFlags=-Od -Oi -Zi -EHsc -GR- -MTd -W4 -nologo
rem set suppressedWarnings=-wd4530
set output=-Fdbin\ -Fobuild\ -Febin\blocks.exe
if not exist "build" md build
if not exist "bin" md bin
goto COMMON

:RELEASE
rem goto COMMON
goto NOCANDO

:COMMON
set source=src\main.cpp
goto BUILD

:BUILD
rem cl %compilerFlags% %suppressedWarnings% %output% %source% user32.lib d3d11.lib d3dcompiler.lib dxguid.lib Winmm.lib
cl %compilerFlags% %suppressedWarnings% %output% %source% user32.lib
goto END

:NOCANDO
echo Unable to perform build
goto END

:END
endlocal