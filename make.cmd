@echo off

set args=%1

goto main

:help
echo usage: ./make.cmd [option]
echo Options:
echo - run: Compile with docker and launch QEMU.
echo - compile: Compile with docker.
echo - clean: Clean project with wsl
echo - qemu: Only launch QEMU
exit

:main
if "%~1"=="" (
	goto help
) else (
	if "%1" == "run" (
		docker-compose up
		qemu-system-i386 -device e1000,netdev=net0 -serial stdio -netdev user,id=net0 -object filter-dump,id=net0,netdev=net0,file=dump.dat -d cpu_reset -D ./log.txt boot.iso
	) else if "%1" == "compile" (
		docker-compose up
	) else if "%1" == "clean" (
		wsl sudo make clean
	) else if "%1" == "qemu" (
		qemu-system-i386 -device e1000,netdev=net0 -serial stdio -netdev user,id=net0 -object filter-dump,id=net0,netdev=net0,file=dump.dat -d cpu_reset -D ./log.txt boot.iso
	)
)