@echo off

if not exist build (
  cmake -H. -Bbuild -G "Visual Studio 17 2022" -A x64 -DBUILD_JNI=ON
)

cmake --build build --config Debug
