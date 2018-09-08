# re-CPM (formerly 'XCPM'): CP/M re-implementation for Unix/Windows and (later) for 8 bit systems as well
# Copyright (C)2016,2018 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>

ALL_ARCHS	= NATIVE WIN32 WIN64

CC_NATIVE	= gcc
CC_WIN32	= i686-w64-mingw32-gcc
CC_WIN64	= x86_64-w64-mingw32-gcc
STRIP_NATIVE	= strip
STRIP_WIN32	= i686-w64-mingw32-strip
STRIP_WIN64	= x86_64-w64-mingw32-strip

DATECODE	= $(shell date '+%Y%m%d%H%M')

PRG_NATIVE	= recpm
PRG_WIN32	= recpm.exe
PRG_WIN64	= recpm_64.exe
PRG_ALL		= $(PRG_NATIVE) $(PRG_WIN32) $(PRG_WIN64)
GENFLAGS_COMMON	= -std=gnu11 -Ofast -fno-common -falign-functions=16 -falign-loops=16 -ffast-math -DDATECODE=\"$(DATECODE)\"
GENFLAGS_NATIVE	= $(GENFLAGS_COMMON)
GENFLAGS_WIN32	= $(GENFLAGS_COMMON)
GENFLAGS_WIN64	= $(GENFLAGS_COMMON)
CFLAGS_NATIVE	= $(GENFLAGS_NATIVE) -I. -Wall -pipe -g
CFLAGS_WIN32	= $(GENFLAGS_WIN32)  -I. -Wall -pipe
CFLAGS_WIN64	= $(GENFLAGS_WIN64)  -I. -Wall -pipe
LDFLAGS_NATIVE  = $(GENFLAGS_NATIVE) -lreadline
LDFLAGS_WIN32	= $(GENFLAGS_WIN32)  -mconsole
LDFLAGS_WIN64	= $(GENFLAGS_WIN64)  -mconsole

OBJPREFIX	= objs/$(ARCH)-

SRCS_COMMON	= main.c hardware.c shell.c exec.c cpm.c
SRCS_NATIVE	= $(SRCS_COMMON)
SRCS_WIN32	= $(SRCS_COMMON)
SRCS_WIN64	= $(SRCS_COMMON)
SRCS		= $(SRCS_$(ARCH))

OBJS		= $(addprefix $(OBJPREFIX), $(SRCS:.c=.o))


all:
	@for a in $(ALL_ARCHS) ; do $(MAKE) arch-build ARCH=$$a || exit 1 ; done

dist:
	$(MAKE) all
	$(MAKE) strip
	rm -f recpm.zip
	zip recpm.zip $(PRG_ALL) README.md LICENSE
	cat recpm.zip | ssh download.lgb.hu ".download.lgb.hu-files/pump recpm.zip-`date '+%Y%m%d%H%M%S'`"

dep:
	@for a in $(ALL_ARCHS) ; do $(MAKE) arch-depend ARCH=$$a || exit 1 ; done

arch-build:
	@if [ ! -s objs/$(ARCH).depend -o Makefile -nt objs/$(ARCH).depend ]; then $(MAKE) arch-depend ARCH=$(ARCH) || exit 1 ; fi
	@echo "*** Building for $(ARCH) as $(PRG_$(ARCH))"
	@$(MAKE) $(PRG_$(ARCH)) ARCH=$(ARCH)

arch-depend:
	@echo "*** Dependency for $(ARCH)"
	$(CC_$(ARCH)) -MM $(CFLAGS_$(ARCH)) $(SRCS) | awk '/^[^.:\t ]+\.o:/ { print "$(OBJPREFIX)" $$0 ; next } { print }' > objs/$(ARCH).depend

arch-strip: $(PRG_$(ARCH))
	$(STRIP_$(ARCH)) $(PRG_$(ARCH))

$(OBJPREFIX)%.o: %.c Makefile
	$(CC_$(ARCH)) $(CFLAGS_$(ARCH)) -c -o $@ $<

$(PRG_$(ARCH)): $(OBJS) objs/$(ARCH)-HELP_TXTDB.o Makefile
	$(CC_$(ARCH)) -o $(PRG_$(ARCH)) $(OBJS) objs/$(ARCH)-HELP_TXTDB.o $(LDFLAGS_$(ARCH))

clean:
	rm -f objs/*.* $(PRG_ALL) recpm.zip

strip:
	@for a in $(ALL_ARCHS) ; do make arch-strip ARCH=$$a || exit 1 ; done

objs/HELP.c: help/*.md ./help Makefile
	cat help/*.md | sed 's/^#\s*/#/;s/\\/\\\\/g;s/"/\\"/g;s/^/\"/;s/\s*$$/\\n\"/;1s/^/const char help_md[] = \"\\n\"\n/;$$s/$$/;/' > $@

objs/$(ARCH)-HELP_TXTDB.o: objs/HELP.c Makefile
	$(CC_$(ARCH)) $(CFLAGS_$(ARCH)) -c -o $@ $<

.PHONY: strip test clean all arch-build arch-depend arch-strip dist dep

ifneq ($(wildcard objs/$(ARCH).depend),)
include objs/$(ARCH).depend
endif
