@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

set "CLANG_FORMAT="

:: 1) PATH
for /f "delims=" %%f in ('where clang-format 2^>nul') do (
  set "CLANG_FORMAT=%%f"
  goto :found
)

:: 2) VS 2022 bundled LLVM
for %%v in (Community Professional Enterprise) do (
  set "BASE=C:\Program Files\Microsoft Visual Studio\2022\%%v\VC\Tools\Llvm\x64\bin"
  if exist "!BASE!\clang-format.exe" (
    set "CLANG_FORMAT=!BASE!\clang-format.exe"
    goto :found
  )
)

:: 3) LLVM default install
if exist "C:\Program Files\LLVM\bin\clang-format.exe" (
  set "CLANG_FORMAT=C:\Program Files\LLVM\bin\clang-format.exe"
  goto :found
)

echo ERROR: clang-format not found on PATH or in known locations.
echo Install LLVM or run from a Visual Studio Developer Command Prompt.
exit /b 1

:found
echo Formatting C++ sources with !CLANG_FORMAT!...

set "DIRS="
set "DIRS=%DIRS% src\*.cpp"
set "DIRS=%DIRS% src\Properties\*.cpp"
set "DIRS=%DIRS% src\Util\*.cpp"
set "DIRS=%DIRS% src\capi\*.cpp"
set "DIRS=%DIRS% src\jni\*.cpp"
set "DIRS=%DIRS% include\wz\*.h"
set "DIRS=%DIRS% include\wz\Properties\*.h"
set "DIRS=%DIRS% include\wz\Util\*.h"
set "DIRS=%DIRS% tests\*.cpp"

"!CLANG_FORMAT!" -i --style=file !DIRS!

echo Done.
endlocal
