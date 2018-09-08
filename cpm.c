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
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include "common.h"
#include "hardware.h"
#include "cpm.h"
#include "exec.h"

#ifdef _WIN32
#include <windows.h>
#endif



static Uint16 dma_address = 0x80;

#define MAX_OPEN_FILES 1000


struct fcb_st {
	int addr;	// Z80 FCB address
	FILE *fp;	// assigned host file
	int pos;	// file pointer value
};
static struct fcb_st fcb_table[MAX_OPEN_FILES];


struct cpm_drive_st cpm_drives[CPM_DRIVES_MAX];
int current_cpm_drive = 0;

char cpm_last_errmsg[1024];

#define SET_ERRMSG(...)	snprintf(cpm_last_errmsg, sizeof cpm_last_errmsg, __VA_ARGS__)



int cpm_drive_logon ( int drive, const char *host_path )
{
	if (drive < 0 || drive >= CPM_DRIVES_MAX) {
		SET_ERRMSG("%s: Invalid drive requested: #%d", __func__, drive);
		return -1;
	}
	if (cpm_drives[drive].host_path[0]) {
		if (chdir(cpm_drives[drive].host_path)) {
			SET_ERRMSG("%s: cannot pre-chdir(): %s", __func__, strerror(errno));
			return -1;
		}
	}
	char resolved_path[PATH_MAX];
	if (
#ifdef _WIN32
		GetFullPathName(host_path, PATH_MAX, resolved_path, NULL)
#else
		realpath(host_path, resolved_path)
#endif
		&& !chdir(resolved_path)
	) {
		strcpy(cpm_drives[drive].host_path, resolved_path);
		int pathsize = strlen(cpm_drives[drive].host_path);
		if (cpm_drives[drive].host_path[pathsize - 1] != DIRSEPCHR)
			strcpy(cpm_drives[drive].host_path + pathsize, DIRSEPSTR);
		cpm_drives[drive].read_only = 0;
		//show_drive_assignment(drive);
		//printf("Drive %c: logged on to %s\n", drive + 'A', resolved_path);
		return 0;
	}
	SET_ERRMSG("%s: failure with path resolution or using it: %s", __func__, strerror(errno));
	return -1;
}


int cpm_select_drive ( int drive )
{
	if (drive < 0 || drive >= CPM_DRIVES_MAX) {
		SET_ERRMSG("Invalid drive #%d to set", drive);
		return -1;
	}
	if (!cpm_drives[drive].host_path[0]) {
		SET_ERRMSG("Not-logged on drive (#%d %c:) cannot be set as default", drive, drive + 'A');
		return -1;
	}
	current_cpm_drive = drive;
	return 0;
}





void write_filename_to_fcb ( Uint16 fcb_addr, const char *fn )
{
	char *fcb = (char*)memory + fcb_addr, *p;
	*(fcb++) = 0;
	memset(fcb, 32, 8 + 3);
	if (!*fn)
		return;
	p = strchr(fn, '.');
	if (p) {
		memcpy(fcb, fn, p - fn >= 8 ?  8 : p - fn);
		memcpy(fcb + 8, p + 1, strlen(p + 1) >= 3 ? 3 : strlen(p + 1));
	} else
		memcpy(fcb, fn, strlen(fn) > 8 ? 8 : strlen(fn));
}



/* Intitialize CP/M C implementation */
int recpm_init ( void  )
{
	for (int a = 0; a < CPM_DRIVES_MAX; a++) {
		cpm_drives[a].host_path[0] = '\0';
		cpm_drives[a].read_only = 0;
	}
	/* Our ugly FCB to host file handle table ... */
	for (int a = 0; a < MAX_OPEN_FILES; a++)
		fcb_table[a].addr = -1;
	cpm_last_errmsg[0] = '\0';
	dma_address = 0x100;
	return 0;
}


void cpm_bios_syscall ( int func )
{
	DEBUG("CBIOS CALL %d\n", func);
	switch (func) {
		case  0:
			CPMPRG_STOP(0, "Exit via BIOS BOOT vector");
			break;
		case  1:
			CPMPRG_STOP(0, "Exit via BIOS WBOOT vector");
			break;
		default:
			CPMPRG_STOP(1, "Not implemented/unknown BIOS call #%d", func);
			break;
	}
}



//             111
//   0123456789012
//    FILENAMEext
static void fcb_to_filename ( Uint16 fcb_addr, char *fn )
{
	int a = 0;
	while (a < 11) {
		char c = memory[++a + fcb_addr] & 127;
		if (c <= 32) {
			if (a == 9) {
				*(fn - 1) = 0;	// empty extension, delete the '.' char was placed for basename/ext separation
				return;		// and end of work
			}
			if (a > 9)
				break;
			//*(fn++) = '.';
			a = 8;	// this will be pre-incremented in the next iteration
		} else {
			if (a == 9)
				*(fn++) = '.';
			*(fn++) = (c >= 'a' && c <= 'z') ? c - 0x20 : c;
		}
	}
	*fn = 0;
}



static struct fcb_st *fcb_search ( int fcb_addr )
{
	int a;
	for (a = 0; a < MAX_OPEN_FILES; a++)
		if (fcb_table[a].addr == fcb_addr)
			return &fcb_table[a];
	return NULL;
}



static struct fcb_st *fcb_free ( Uint16 fcb_addr )
{
	struct fcb_st *p = fcb_search(fcb_addr);
	if (p) {
		DEBUG("EMU: fcb_free found open file, closing it %p\n", p->fp);
		fclose(p->fp);
		p->addr = -1;
		return p;
		
	}
	return NULL;
}




static struct fcb_st *fcb_alloc ( Uint16 fcb_addr, FILE *fp )
{
	struct fcb_st *p = fcb_free(fcb_addr);	// we use this to also FREE if was used before for whatever reason ...
	if (!p) {	// if not used before ...
		p = fcb_search(-1);	// search for empty slot
		if (!p)
			return NULL;	// to many open files, etc?!
	}
	p->addr = fcb_addr;
	p->pos = 0;
	p->fp = fp;
	return p;
}




static int bdos_open_file ( Uint16 fcb_addr, int is_create )
{
	FILE *f;
	char fn[14];
	struct fcb_st *p;
	const char *FUNC = is_create ? "CREATE" : "OPEN";
	fcb_to_filename(fcb_addr, fn);
	DEBUG("CPM: %s: filename=\"%s\" FCB=%04Xh create?=%d\n", FUNC, fn, fcb_addr, is_create);
	// TODO: no unlocked mode, no read-only mode ...
	if (is_create) {
		f = fopen(fn, "rb");
		if (f) {
			fclose(f);
			CPMPRG_STOP(1, "BDOS FILE CREATE FUNC: file %s already exists.", fn);
			return 1;
		}
		f = fopen(fn, "w+b");
	} else
		f = fopen(fn, "r+b");
	if (!f) {
		DEBUG("CPM: %s: cannot open file ...\n", FUNC);
		DEBUG("CPM: DEBUG: FCB file name area: %02X %02X %02X %02X %02X %02X %02X %02X . %02X %02X %02X\n",
			memory[fcb_addr + 1], memory[fcb_addr + 2], memory[fcb_addr + 3], memory[fcb_addr + 4],
			memory[fcb_addr + 5], memory[fcb_addr + 6], memory[fcb_addr + 7], memory[fcb_addr + 8],
			memory[fcb_addr + 9], memory[fcb_addr + 10], memory[fcb_addr + 11]
		);
		return 1;
	}
	p = fcb_alloc(fcb_addr, f);
	if (!p) {
		fclose(f);
		DEBUG("CPM: %s: cannot allocate FCB translation structure for emulation :-(\n", FUNC);
		return 1;
	}
	// OK, everything seems to be OK! Let's fill the rest of the FCB ...
	DEBUG("CPM: %s: seems to be OK :-)\n", FUNC);
	memset(memory + fcb_addr + 0x10, 0, 20);
	return 0;
}



static int bdos_delete_file ( Uint16 fcb_addr )
{
	char fn[14];
	int a;
	fcb_free(fcb_addr);
	// FIXME: ? characters are NOT supported!!!
	fcb_to_filename(fcb_addr, fn);
	DEBUG("CPM: DELETE: filename=\"%s\" FCB=%04Xh\n", fn, fcb_addr);
	a = remove(fn);
	DEBUG("CPM: DELETE: remove operation result = %d\n", a);
	return a;
}


static int bdos_read_next_record ( Uint16 fcb_addr )
{
	int a;
	struct fcb_st *p = fcb_search(fcb_addr);
	DEBUG("CPM: READ: FCB=%04Xh VALID?=%s DMA=%04Xh\n", fcb_addr, p ? "YES" : "NO", dma_address);
	if (!p)
		return 9;	// invalid FCB
	a = fread(memory + dma_address, 1, 128, p->fp);
	DEBUG("CPM: READ: read result is %d (0 = EOF)\n", a);
	if (a <= 0)
		return 1;	// end of file
	if (a < 128)		// fill the rest of the buffer, if not a full 128 bytes record could be read
		memset(memory + dma_address + a, 0, 128 - a);
	return 0;
}


static int bdos_random_access_read_record ( Uint16 fcb_addr )
{
	int offs, a;
	struct fcb_st *p = fcb_search(fcb_addr);
	DEBUG("CPM: RANDOM-ACCESS-READ: FCB=%04Xh VALID?=%s DMA=%04Xh\n", fcb_addr, p ? "YES" : "NO", dma_address);
	if (!p)
		return 9;	// invalid FCB
	offs = 128 * (memory[fcb_addr + 0x21] | (memory[fcb_addr + 0x22] << 8));	// FIXME: is this more than 16 bit?
	DEBUG("CPM: RANDOM-ACCESS-READ: file offset = %d\n", offs);
	if (fseek(p->fp, offs, SEEK_SET) < 0) {
		DEBUG("CPM: RANDOM-ACCESS-READ: Seek ERROR!\n");
		return 6;	// out of range
	}
	DEBUG("CPM: RANDOM-ACCESS-READ: Seek OK. calling bdos_read_next_record for read ...\n");
	a = bdos_read_next_record(fcb_addr);
	fseek(p->fp, offs, SEEK_SET);	// re-seek. According to the spec sequential read should be return with the same record. Odd enough this whole FCB mess ...
	return a;
}


static int bdos_write_next_record ( Uint16 fcb_addr )
{
	int a;
	struct fcb_st *p = fcb_search(fcb_addr);
	DEBUG("CPM: WRITE: FCB=%04Xh VALID?=%s DMA=%04Xh\n", fcb_addr, p ? "YES" : "NO", dma_address);
	if (!p)
		return 9;	// invalid FCB
	a = fwrite(memory + dma_address, 1, 128, p->fp);
	DEBUG("CPM: WRITE: write result is %d\n", a);
	if (a != 128)
		return 2;	// report disk full in case of write problem ...
	return 0;
}




static int bdos_close_file ( Uint16 fcb_addr )
{
	struct fcb_st *p = fcb_search(fcb_addr);
	DEBUG("CPM: CLOSE: FCB=%04Xh VALID?=%s\n", fcb_addr, p ? "YES" : "NO");
	fcb_free(fcb_addr);
	return 0; // who cares!!!!! :)
}



static void bdos_buffered_console_input ( Uint16 buf_addr )
{
	char buffer[256];
	DEBUG("CPM: BUFCONIN: console input, buffer = %04Xh\n", buf_addr);
	memory[buf_addr] = 0;
	if (fgets(buffer, sizeof buffer, stdin)) {
		char *p = buffer;
		char *q = (char*)memory + buf_addr + 1;
		while (*p && *p != 13 && *p != 10 && memory[buf_addr] < 255) {
			*(q++) = *(p++);
			memory[buf_addr]++;
		}
		DEBUG("CPM: BUFCONIN: could read %d bytes\n", memory[buf_addr]);
		
	} else {
		DEBUG("CPM: BUFCONIN: cannot read, pass back zero bytes!\n");
	}
}


static void bdos_output_string ( Uint16 addr )
{
	char *p = (char*)memory + addr;
	while (*p != '$')
		putchar(*(p++));
}







void cpm_bdos_syscall ( int func )
{
	switch (func) {
		case 2:		// console output
			putchar(Z80_E);
			//fsync(stdout);
			break;
		case 9:		// Output '$' terminated string
			bdos_output_string(Z80_DE);
			break;
		case 10:	// console input - but it's not emulated ...
			bdos_buffered_console_input(Z80_DE);
			break;
		case 12:	// Get version
			Z80_A = Z80_L = 0x22;	// version 2.2
			Z80_B = Z80_H = 0;	// system type
			break;
		case 13:	// Reset disks
			Z80_A = Z80_L = 0;
			break;
		case 14:	// Select disk, we just fake an OK answer, how cares about drives :)
			Z80_A = Z80_L = 0;
			break;
		case 15: 	// Open file, the horror begins :-/ Nobody likes FCBs, honestly ...
			Z80_A = Z80_L = bdos_open_file(Z80_DE, 0) ? 0xFF : 0;
			break;
		case 16:	// CLose file
			Z80_A = Z80_L = bdos_close_file(Z80_DE) ? 0xFF : 0;
			break;
		case 19:	// Delete file
			Z80_A = Z80_L = bdos_delete_file(Z80_DE) ? 0xFF : 0;
			break;
		case 20:	// read next record ...
			Z80_A = Z80_L = bdos_read_next_record(Z80_DE);
			break;
		case 21:	// write next record ...
			Z80_A = Z80_L = bdos_write_next_record(Z80_DE);
			break;
		case 22:	// Create file: tricky, according to the spec, if file existed before, user app will be stopped, or whatever ...
			Z80_A = Z80_L = bdos_open_file(Z80_DE, 1) ? 0xFF : 0;
			break;
		case 25:	// Return current drive. We just fake 0 (A)
			Z80_A = Z80_L = 0;
			break;
		case 26:	// Set DMA address
			DEBUG("CPM: SETDMA: to %04Xh\n", Z80_DE);
			dma_address = Z80_DE;
			break;
		case 33:	// Random access read record (note: file pointer should be modified that sequential read reads the SAME [??] record then!!!)
			Z80_A = Z80_L = bdos_random_access_read_record(Z80_DE);
			break;
		default:
			CPMPRG_STOP(-1, "Unimplemented BDOS call #%d", func);
			break;
	}
}
