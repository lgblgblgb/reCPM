/* This is a MINIMAL CP/M emulator. That is, it emulates a Z80 (though 8080 would be
   enough for most CP/M software), the BDOS and the CBIOS as well, and just expects
   a CP/M program to load and run. It was written *ONLY FOR* running some ancient
   tools, namely M80 assembler and L80 linker (they don't seem to have modern versions
   for non-CP/M OSes). The emulator has BDOS/CBIOS functions implemented and in a way
   that is enough for these tools! Maybe this can change in the future, but this is the
   current situation! Stderr should be redirected, as those are DEBUG messages.

   Copyright (C)2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>

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
#include "hardware.h"

#define ED_TRAP_OPCODE	0xBC
#define CBIOS_JUMP_TABLE_ADDR	0xFE00
#define CBIOS_ENTRIES		((0x10000 - CBIOS_JUMP_TABLE_ADDR) / 3)
// BDOS entry point
#define BDOS_ENTRY_ADDR		0xFD00


#define DEBUG(...) fprintf(stderr, __VA_ARGS__)


static Uint16 dma_address = 0x80;

#define MAX_OPEN_FILES 1000


struct fcb_st {
	int addr;	// Z80 FCB address
	FILE *fp;	// assigned host file
	int pos;	// file pointer value
};
static struct fcb_st fcb_table[MAX_OPEN_FILES];









static void write_filename_to_fcb ( Uint16 fcb_addr, const char *fn )
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



/* Intitialize CP/M emulation */
static int cpm_init ( int argc, char **argv )
{
	FILE *f;
	int a;
	if (argc < 2) {
		fprintf(stderr, "Usage error: at least one parameter expected, the name of the CP/M program\nAfter that, you can give the switches/etc for the CP/M program itself\n");
		return 1;
	}
	/* Our ugly FCB to host file handle table ... */
	for (a = 0; a < MAX_OPEN_FILES; a++)
		fcb_table[a].addr = -1;
	// memory init
	memset(memory, 0, sizeof memory);
	/* create jump table for CBIOS emulation, actually they're CPU traps, and RET! */
	for (a = 0; a < CBIOS_ENTRIES; a++) {
		memory[CBIOS_JUMP_TABLE_ADDR + a * 3 + 0] = 0xED;
		memory[CBIOS_JUMP_TABLE_ADDR + a * 3 + 1] = ED_TRAP_OPCODE;
		memory[CBIOS_JUMP_TABLE_ADDR + a * 3 + 2] = 0xC9;	// RET opcode ...
	}
	/* create a single trap entry for BDOS emulation */
	memory[BDOS_ENTRY_ADDR + 0] = 0xED;
	memory[BDOS_ENTRY_ADDR + 1] = ED_TRAP_OPCODE;
	memory[BDOS_ENTRY_ADDR + 2] = 0xC9;		// RET opcode ...
	// std CP/M BDOS entry point in the low memory area ...
	memory[5] = 0xC3;	// JP opcode
	memory[6] = BDOS_ENTRY_ADDR & 0xFF;
	memory[7] = BDOS_ENTRY_ADDR >> 8;
	// CP/M CBIOS stuff
	memory[0] = 0xC3;	// JP opcode
	memory[1] = (CBIOS_JUMP_TABLE_ADDR + 3) & 0xFF;
	memory[2] = (CBIOS_JUMP_TABLE_ADDR + 3) >> 8;
	// Disk I/O byte etc
	memory[3] = 0;
	memory[4] = 0;
	// Now fill buffer of the command line
	memory[0x81] = 0;
	memset(memory + 0x5C + 1, 32, 11);
	memset(memory + 0x6C + 1, 32, 11);
	for (a = 2; a < argc; a++) {
		if (a <= 3)
			write_filename_to_fcb(a == 2 ? 0x5C : 0x6C, argv[a]);
		if (memory[0x81])
			strcat((char*)memory + 0x81, " ");
		strcat((char*)memory + 0x81, argv[a]);
		if (strlen((char*)memory + 0x81) > 0x7F) {
			fprintf(stderr, "Too long command line for the CP/M program!\n");
			return 1;
		}
	}
	memory[0x80] = strlen((char*)memory + 0x81);
	// Load program
	f = fopen(argv[1], "rb");
	if (!f) {
		fprintf(stderr, "Cannot open program file: %s\n", argv[1]);
		return 1;
	}
	a = fread(memory + 0x100, 1, BDOS_ENTRY_ADDR - 0x100 + 1, f);
	fclose(f);
	if (a < 10) {
		fprintf(stderr, "Too short CP/M program file: %s\n", argv[1]);
		return 1;
	}
	if (a > 0xC000) {
		fprintf(stderr, "Too large CP/M program file: %s\n", argv[1]);
		return 1;
	}
	Z80_PC = 0x100;
	Z80_SP = BDOS_ENTRY_ADDR;
	DEBUG("*** Starting program: %s with parameters %s\n", argv[1], (char*)memory + 0x81);
	return 0;
}


static void cbios_call ( int func )
{
	switch (func) {
		case -3:
			DEBUG("CPM: BIOS: cold start routine has been called. Exiting ...\n");
			exit(0);
		case  0:
			DEBUG("CPM: BIOS: warm boot routine has been called. Exiting ...\n");
			exit(0);
		default:
			DEBUG("CPM: BIOS: FATAL: unsupported call, function offset: %d\n", func);
			exit(1);
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
			DEBUG("CPM: FATAL: FILE CREATE FUNC, but file %s existed before, stopping!\n", fn);
			exit(1);
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







static void bdos_call ( int func )
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
			DEBUG("CPM: BDOS: FATAL: unsupported call %d\n", func);
			exit(1);
	}
}




int z80ex_ed_cb(Z80EX_BYTE opcode) {
	//DEBUG("ED trap at %04Xh\n", Z80_PC);
	if (Z80_PC > CBIOS_JUMP_TABLE_ADDR) {
		int div = (Z80_PC - CBIOS_JUMP_TABLE_ADDR) / 3;
		int mod = (Z80_PC - CBIOS_JUMP_TABLE_ADDR) % 3;
		if (mod != 2) {
			DEBUG("CPM: FATAL: unknown trap position in the CBIOS jump table!\n");
			exit(1);
		}
		cbios_call(div * 3 - 3);
	} else if (Z80_PC == BDOS_ENTRY_ADDR + 2) {
		bdos_call(Z80_C);
	} else {
		DEBUG("CPM: FATAL: unknown trap memory location (+2) at %04X\n", Z80_PC);
		exit(1);
	}
	return 0;
}




int main ( int argc, char **argv )
{
	z80ex_init();
	z80ex_reset();
	if (cpm_init(argc, argv))
		return 1;
	for (;;) {
		z80ex_step();
	}
	return 0;
}


