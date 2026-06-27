@echo off

cpplint --recursive --config .cpplint src include
if errorlevel 1 exit /b %errorlevel%

for %%f in (tests\*.cpp) do (
    cpplint --config .cpplint "%%f"
    if errorlevel 1 exit /b %errorlevel%
)
