@echo off
setlocal

echo ================================
echo   CPP - BUILD
echo ================================

cd /d %~dp0

REM DELETE OLD BUILD FOLDER
if exist build (
echo Cleaning old build folder...
rmdir /s /q build
)

REM Recreate build folder
mkdir build
cd build

echo Configuring CMake (Release)...

cmake .. -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
echo.
echo [ERROR] CMake configure failed!
pause
exit /b %errorlevel%
)

echo.
echo Building project...

cmake --build .

if %errorlevel% neq 0 (
echo.
echo [ERROR] Build failed!
pause
exit /b %errorlevel%
)

cd ..
copy /Y .\build\SimplePrinter.exe .\test\SimplePrinter.exe 

echo.
echo ================================
echo Build SUCCESS
echo ================================

echo.
pause
endlocal
