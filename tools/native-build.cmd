@echo off
setlocal DisableDelayedExpansion
set "VSLANG=1033"
call "%~1" -no_logo -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
"%~2" -S "%~3" -B "%~4" -G Ninja "-DCMAKE_MAKE_PROGRAM=%~5" "-DCMAKE_BUILD_TYPE=%~6" "-DXC3_BUILD_TESTS=%~7"
if errorlevel 1 exit /b %errorlevel%
"%~2" --build "%~4" --parallel
exit /b %errorlevel%
