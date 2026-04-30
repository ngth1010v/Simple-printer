@echo off
setlocal

echo =========================================
echo  CPP - RUN
echo =========================================

cd bin
.\SimplePrinter.exe ..\test\test.pdf

echo =========================================
echo  CPP - DONE RUN
echo =========================================

pause
endlocal
