# re-CP/M

(C)2016 Gábor Lénárt (LGB) lgblgblgb@gmail.com

re-CP/M is licensed under the terms of GNU/GPL v2, for more information please
read file LICENSE.

The UNIX/Windows target uses (modified, by me) Z80ex library to emulate
8080/Z80: https://sourceforge.net/projects/z80ex/

re-CP/M tries to implement a CP/M compatible system (v2.2 subset only) for
various targets, including top of Unix/Windows, but for 8 bit systems as well.

Important: this project in its early stage, without too much usable results!
Also, this document can be seriously outdated or simply wrong as well.

There are various projects to write replacament BIOSes for CP/M BDOS to have
CP/M system for various SBC (Single Board Computer) projects, utilizing the
8080 (or Z80) CPU. Also, there are some projects wanting to emulate the CPU
and some minimal hardware to be able to run CP/M (basically the BDOS) on a
modern PC. My goal however is a re-implementation of CP/M including the BDOS.

Note, that I am quite liberal to implement only a subset of CP/M v2.2, which
is enough to run various CP/M softwares, *I* need. I don't say that it's
a fully CP/M compatible ecosystem.

Basically, this repository is created to host my experiments. It includes
a CP/M-alike system built in top of Linux/UNIX or Windows, but also (maybe
later, at least at the time when I write this) for 8 bit systems, like
the Z80 itself, or using software emulation of the 8080 on other
"retro" CPUs to run the CP/M applications, but using native (to the target
CPU) for the CP/M "core" itself (functionality of BDOS and CBIOS).

The suspect confusion of mine about the 8080/Z80 is by intent: on "real"
hardware I prefer/use Z80, though as CP/M should be OK with the 8080 and it's
a more simple CPU, software emulation is often meant to be "8080 compatible"
only in most cases. Also, even with the 8080, I use Z80 asm syntax, since I
find 8080 asm syntax a bit hmmmm "strange".

# Planned targets

* Linux/UNIX and Windows, code in C, Z80 is emulated with Z80ex (though
  traditionally, 8080 is "enough" for CP/M). This target and the emulator
  is also named as "XCPM".

  This target is great way to test CP/M emulation easily while also providing
  CP/M-alike features for modern operating systems. It would be much harder to
  try/test new CP/M implementation coded in assembly for 8 bit systems first.

  A CCP replacement with some "built-in" commands are also planned, but
  basically (without this feature used) XCPM is suitable to invoke a CP/M
  command with the ability to redirect stdin/stdout as it would be a native
  program running on your OS.

* 65xx CPUs, code in 65xx asm, 8080 CPU is software emulated in 65xx asm (no
  Z80 is emulated, only 8080)

  Possible target machines: Commodore 64DTV, Commodore 65, Commodore 128 [?],
  Commodore 64/128 + SuperCPU

* Z80 CPU, code in Z80 asm, no 8080 CPU emulation is needed

  Possible target machines: Z80 SBC projects, Enterprise-128 (for testing
  mainly as its IS-DOS is basically a CP/M compatible system wihtout DR's
  original BDOS)

# Limitations

No low level disk format, or access is supported, and it's not planned either.
The key feature is exactly the possibility to support CP/M applications
transparently using the file system what the "host" provides. This also means,
that CP/M software trying to access disk (or special FCB structure elements
etc) will fail.

Since CP/M 2.2 does not know about "directories", the system mostly uses a
single directory. Though the planned CCP will provide some commands to change
the current directory etc for more comfortable use.

XCPM currently assumes little endian host CPU byte order, that is x86 for
example. It won't work in this form on a CPU with other byte order (you should
define Z80EX_WORDS_BIG_ENDIAN before including z80ex in that case).

# Build instructions for XCPM

Just say 'make'. Note, you need to have gcc and libc development libraries
installed _ALSO_ with Win32 cross compiler on Linux/UNIX. You also needs
GNU (!) make utility. If you don't need the Windows build part, you can edit
Makefile, and line ALL_ARCHS need to be modified to have the only word NATIVE.

