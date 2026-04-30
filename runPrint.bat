@echo off
setlocal

echo =========================================
echo  CPP - RUN
echo =========================================

cd bin
.\SimplePrinter.exe --print ..\test\test.pdf ..\test\test.png

echo =========================================
echo  CPP - DONE RUN
echo =========================================

pause
endlocal
