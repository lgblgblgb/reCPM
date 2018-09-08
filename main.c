/* This is a MINIMAL CP/M emulator. That is, it emulates a Z80 (though 8080 would be
   enough for most CP/M software), the BDOS and the CBIOS as well, and just expects
   a CP/M program to load and run. It was written *ONLY FOR* running some ancient
   tools, namely M80 assembler and L80 linker (they don't seem to have modern versions
   for non-CP/M OSes). The emulator has BDOS/CBIOS functions implemented and in a way
   that is enough for these tools! Maybe this can change in the future, but this is the
   current situation! Stderr should be redirected, as those are DEBUG messages.

   Copyright (C)2016,2018 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "hardware.h"
#include "shell.h"
#include "cpm.h"
#include "exec.h"


#include <limits.h>
#include <libgen.h>



int main ( int argc, char **argv )
{
	z80ex_init();
	recpm_init();
	console_init();
	if (argc < 2) {
		console_title("[SHELL]");
		return shell_main();
	} else {
		char pathinfo[PATH_MAX];
		strcpy(pathinfo, argv[1]);
		char *dir = dirname(pathinfo);
		int ret = cpmprg_load(argv[1], argc - 2, argv + 2);
		if (ret) {
			show_termination_error(stderr);
			return ret;
		}
		if (cpm_drive_logon(0, dir)) {
			fprintf(stderr, "Cannot logon A: to %s: %s\n", dir, cpm_last_errmsg);
			return -1;
		}
		console_title("USERPRG");
		ret = cpmprg_execute();
		if (ret)
			show_termination_error(stderr);
		return ret;
	}
}
