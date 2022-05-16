@echo off

echo.
echo Trying to use WSL to run makefile and create iso file.
echo - All output files will still be in /bin/ and boot.iso
echo   in main directory.
echo.
echo - To start with QEMU use ./win.cmd run
echo.
set /p id="Press any key to start compiling."

set args=%1

wsl make

if "%1" == "run" (
	qemu-system-i386 boot.iso
)
