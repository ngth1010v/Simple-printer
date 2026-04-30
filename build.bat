@echo off
setlocal

echo ================================
echo   CPP - BUILD
echo ================================

cd /d %~dp0

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
mkdir bin
copy /Y .\build\SimplePrinter.exe .\bin\SimplePrinter.exe 
copy /y .\external\pdfium\bin\pdfium.dll .\bin\pdfium.dll

echo.
echo ================================
echo Build SUCCESS
echo ================================

echo.
pause
endlocal
