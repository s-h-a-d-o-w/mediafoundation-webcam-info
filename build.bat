@echo off
setlocal

set SRC=main.cpp
set OUT=main.exe

where cl >nul 2>nul
if %errorlevel%==0 (
  echo Compiling...
  cl /nologo /EHsc /std:c++17 /O2 "%SRC%" /Fe:"%OUT%" /link mfplat.lib mf.lib mfreadwrite.lib mfuuid.lib ole32.lib
  if %errorlevel% neq 0 exit /b %errorlevel%
  if exist *.obj del /q *.obj >nul 2>nul
  echo Build complete: "%OUT%"
  exit /b 0
)

echo Error: MSVC compiler not found - please install MSVC or run from a Developer Command Prompt.
exit /b 1
