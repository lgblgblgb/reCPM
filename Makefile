all:
	true

cpm-minimal-emulator.exe:
	rm -f cpm-minimal-emulator.exe
	i686-w64-mingw32-gcc -Wall -Ofast -ffast-math -pipe -mconsole -o cpm-minimal-emulator.exe cpm-minimal-emulator.c
	i686-w64-mingw32-strip cpm-minimal-emulator.exe
	ls -l cpm-minimal-emulator.exe

cpm-minimal-emulator:
	rm -f cpm-minimal-emulator
	gcc -Wall -Ofast -ffast-math -pipe -o cpm-minimal-emulator cpm-minimal-emulator.c
	strip cpm-minimal-emulator
	ls -l cpm-minimal-emulator

clean:
	rm -f cpm-minimal-emulator.exe cpm-minimal-emulator

.PHONY: test clean all

