@echo off
setlocal

if /I "%1"=="--profile" set profileEnable=-D_PROFILE_
if /I "%1"=="/profile" set profileEnable=-D_PROFILE_
if /I "%2"=="--profile" set profileEnable=-D_PROFILE_
if /I "%2"=="/profile" set profileEnable=-D_PROFILE_

if /I "%1"=="" goto DEBUG
if /I "%1"=="--debug" goto DEBUG
if /I "%1"=="/debug" goto DEBUG

if /I "%1"=="--release" goto RELEASE
if /I "%1"=="/release" goto RELEASE
if /I "%1"=="/profile" goto RELEASE


:PROFILE
set profileEnable=-D_PROFILE_
goto RELEASE

:DEBUG
set compilerFlags=-MP -Od -Oi -Zi -EHsc -GR- -MTd -W4 -nologo -D_DEBUG_ -D_CRT_SECURE_NO_WARNINGS %profileEnable%
rem set suppressedWarnings=-wd4530
set output=-Fdbin\ -Fobuild\ -Febin\blocks.exe
if not exist "build" md build
if not exist "bin" md bin
goto COMMON

:RELEASE
set compilerFlags=-MP -Ox -Oi -EHsc -GR- -MT -W4 -nologo -D_RELEASE_ -DNDEBUG -D_CRT_SECURE_NO_WARNINGS %profileEnable%
rem set suppressedWarnings=-wd4530
set output=-Fdbin\ -Fobuild\ -Febin\blocks.exe
goto COMMON

:COMMON
set source=src\main.cpp src\lib\obj_importer.cpp src\lib\DDSTextureLoader.cpp src\lib\win32_file.cpp src\lib\tokenstream.cpp src\game.cpp src\renderer.cpp src\world.cpp src\utils.cpp src\input.cpp src\collision.cpp
goto BUILD

:BUILD
rem cl %compilerFlags% %suppressedWarnings% %output% %source% user32.lib d3d11.lib d3dcompiler.lib dxguid.lib Winmm.lib
cl %compilerFlags% %suppressedWarnings% %output% %source% user32.lib d3d11.lib dxguid.lib DXGI.lib D3DCompiler.lib
goto END

:NOCANDO
echo Unable to perform build
goto END

:END
endlocal
