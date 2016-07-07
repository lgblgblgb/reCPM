# re-CPM and XCPM: CP/M re-implementation for Unix/Windows and (later) for 8 bit systems as well
# Copyright (C)2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>

ALL_ARCHS	= NATIVE WIN32 WIN64

CC_NATIVE	= gcc
CC_WIN32	= i686-w64-mingw32-gcc
CC_WIN64	= x86_64-w64-mingw32-gcc
STRIP_NATIVE	= strip
STRIP_WIN32	= i686-w64-mingw32-strip
STRIP_WIN64	= x86_64-w64-mingw32-strip

PRG_NATIVE	= xcpm
PRG_WIN32	= xcpm.exe
PRG_WIN64	= xcpm_64.exe
PRG_ALL		= $(PRG_NATIVE) $(PRG_WIN32) $(PRG_WIN64)
GENFLAGS_NATIVE	= -Ofast -fno-common -falign-functions=16 -falign-loops=16 -ffast-math
GENFLAGS_WIN32	= -Ofast -fno-common -falign-functions=16 -falign-loops=16 -ffast-math
GENFLAGS_WIN64	= -Ofast -fno-common -falign-functions=16 -falign-loops=16 -ffast-math
CFLAGS_NATIVE	= $(GENFLAGS_NATIVE) -I. -Wall -pipe
CFLAGS_WIN32	= $(GENFLAGS_WIN32)  -I. -Wall -pipe
CFLAGS_WIN64	= $(GENFLAGS_WIN64)  -I. -Wall -pipe
LDFLAGS_NATIVE  = $(GENFLAGS_NATIVE)
LDFLAGS_WIN32	= $(GENFLAGS_WIN32)  -mconsole
LDFLAGS_WIN64	= $(GENFLAGS_WIN64)  -mconsole

OBJPREFIX	= objs/$(ARCH)-

SRCS_COMMON	= xcpm.c hardware.c
SRCS_NATIVE	= $(SRCS_COMMON)
SRCS_WIN32	= $(SRCS_COMMON)
SRCS_WIN64	= $(SRCS_COMMON)
SRCS		= $(SRCS_$(ARCH))

OBJS		= $(addprefix $(OBJPREFIX), $(SRCS:.c=.o))



all:
	@mkdir -p objs
	@for a in $(ALL_ARCHS) ; do $(MAKE) arch-build ARCH=$$a ; done

arch-build:
	@echo "*** Building for $(ARCH) as $(PRG_$(ARCH))"
	@if [ ! -s .depend.$(ARCH) ]; then $(MAKE) arch-depend ARCH=$(ARCH) ; fi
	@$(MAKE) $(PRG_$(ARCH)) ARCH=$(ARCH)

arch-depend:
	$(CC_$(ARCH)) -MM $(CFLAGS_$(ARCH)) $(SRCS) | awk '/^[^.:\t ]+\.o:/ { print "$(OBJPREFIX)" $$0 ; next } { print }' > .depend.$(ARCH)

arch-strip: $(PRG_$(ARCH))
	$(STRIP_$(ARCH)) $(PRG_$(ARCH))

$(OBJPREFIX)%.o: %.c Makefile
	$(CC_$(ARCH)) $(CFLAGS_$(ARCH)) -c -o $@ $<

$(PRG_$(ARCH)): $(OBJS) Makefile
	$(CC_$(ARCH)) $(LDFLAGS_$(ARCH)) -o $(PRG_$(ARCH)) $(OBJS)

clean:
	rm -f objs/* .depend.* $(PRG_ALL)

strip:
	@for a in $(ALL_ARCHS) ; do make arch-strip ARCH=$$a ; done

.PHONY: strip test clean all arch-build arch-depend arch-strip

ifneq ($(wildcard .depend.$(ARCH)),)
include .depend.$(ARCH)
endif
