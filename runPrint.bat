@echo off
setlocal

echo =========================================
echo  CPP - RUN
echo =========================================

cd bin
.\SimplePrinter.exe --print test.txt

echo =========================================
echo  CPP - DONE RUN
echo =========================================

pause
endlocal
