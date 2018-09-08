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

#ifndef __RECPM_CPM_H_INCLUDED
#define __RECPM_CPM_H_INCLUDED

#include <limits.h>

#define CPM_DRIVES_MAX 16

struct cpm_drive_st {
    char host_path[PATH_MAX];
    int read_only;
};

extern struct cpm_drive_st cpm_drives[CPM_DRIVES_MAX];
extern int current_cpm_drive;
extern char cpm_last_errmsg[];

extern int  recpm_init ( void );
extern void cpm_bios_syscall ( int func );
extern void cpm_bdos_syscall ( int func );
extern void write_filename_to_fcb ( Uint16 fcb_addr, const char *fn );
extern int  cpm_drive_logon ( int drive, const char *host_path );
extern int  cpm_select_drive ( int drive );

#endif
