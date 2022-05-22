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
	qemu-system-i386 -device e1000,netdev=net0 -netdev user,id=net0,hostfwd=tcp::5555-:22 -object filter-dump,id=net0,netdev=net0,file=dump.dat boot.iso
)
echo -netdev user,id=u1 -device e1000,netdev=u1 -object filter-dump,id=f1,netdev=u1,file=dump.dat