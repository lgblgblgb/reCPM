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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include "common.h"
#include "hardware.h"
#include "cpm.h"
#include "exec.h"

#define ED_TRAP_OPCODE	0xBC
#define CBIOS_JUMP_TABLE_ADDR	0xFE00
#define CBIOS_ENTRIES		((0x10000 - CBIOS_JUMP_TABLE_ADDR) / 3)
// BDOS entry point
#define BDOS_ENTRY_ADDR		0xFD00


#define Z80_HZ			6000000
#define	CPU_THROT_SCHED_HZ	100
#define cpu_throttle_cycles	0


static int termination_code;
static char termination_msg[1024];


void show_termination_error ( FILE *stream )
{
	fprintf(stream, "[%d]%s\n", termination_code, termination_msg);
}



static int ed_trap_processor ( void ) {
	DEBUG("ED trap at fault_pc=%04Xh PC_now=%04X\n", z80ev.fault_pc, Z80_PC);
	if (z80ev.fault_pc >= CBIOS_JUMP_TABLE_ADDR) {
		if ((z80ev.fault_pc - CBIOS_JUMP_TABLE_ADDR) % 3) {
			CPMPRG_STOP(-1, "%s(): unknown trap position in the CBIOS jump table", __func__);
			return 0;
		}
		cpm_bios_syscall((z80ev.fault_pc - CBIOS_JUMP_TABLE_ADDR) / 3);
	} else if (z80ev.fault_pc == BDOS_ENTRY_ADDR) {
		cpm_bdos_syscall(Z80_C);
	} else {
		CPMPRG_STOP(1, "%s(): unknown trap memory location (+2) at %04X", __func__, z80ev.fault_pc);
		return 0;
	}
	return 0;
}


static void memory_protection_trap_processor ( void ) {
	CPMPRG_STOP(1, "Memory write protection failure at PC=%04X writing address %04Xh with data %02Xh. Allowed user memory writes: %04Xh-%04Xh",
		z80ev.fault_pc, z80ev.fault_addr, z80ev.fault_data,
		z80ev.user_mem_first_byte, z80ev.user_mem_last_byte
	);
}


static void cpu_throttling ( int cycles )
{
	static struct timeval old_time = {0, 0};
	static int sleep_time = 0;
	struct timeval new_time;
	gettimeofday(&new_time, NULL);
	int usec_emu = new_time.tv_sec - old_time.tv_sec;
	if (usec_emu < 2) {
		usec_emu = usec_emu * 1000000 + (new_time.tv_usec - old_time.tv_usec);
		int usec_z80 = 1000000 * cycles / Z80_HZ;
		sleep_time += usec_z80 - usec_emu;
		if (sleep_time < -250000 || sleep_time > 250000)
			sleep_time = 0;
		else if (sleep_time > 10)
			usleep(sleep_time);
	}
	gettimeofday(&old_time, NULL);
	sleep_time -= (old_time.tv_sec - new_time.tv_sec) * 1000000 + (old_time.tv_usec - new_time.tv_usec);
}



// Ecexute Z80 CP/M program.
// Warning, this does NOT initialize anything, even not Z80 PC!
// Also, there is no memory initialized, default FCBs filled, BDOS/BIOS vector put, etc etc ...
// Just calling this without proper preparations would result in something totally insane.
// Also, surely, it expects the program being in the memory already ...
// You can use it to continue emulation for example ...
// With "installed" in-memory ED-traps, Z80 emulation will actually find its way to pass
// BDOS/BIOS calls into our implementation. For that, check out Z80_EVENT_TRAP below,
// and the function it calls. That is responsible to resolve the traps and call BIOS/BDOS
// implementation of us, coded in C in reCPM (basically see source cpm.c for those)

int cpmprg_z80_execute ( void )
{
	int cycles_left = cpu_throttle_cycles;
	termination_code = -1;
	strcpy(termination_msg, "internal error: termination code/msg is not filled");
	do {
		z80ev.event = 0;
		if (LIKELY(!cpu_throttle_cycles)) {
			while (LIKELY(!z80ev.event))
				z80ex_step();
		} else {
			while (LIKELY(!z80ev.event && cycles_left >= 0))
				cycles_left -= z80ex_step();
			if (UNLIKELY(cycles_left < 0 && !z80ev.event))
				z80ev.event = Z80_EVENT_TICK;
		}
		switch (z80ev.event) {
			case Z80_EVENT_ED_TRAP:
				// this will call BIOS/BDOS after all ...
				ed_trap_processor();
				break;
			case Z80_EVENT_MPROTECT:
				memory_protection_trap_processor();
				break;
			case Z80_EVENT_TICK:
				cpu_throttling(cpu_throttle_cycles - cycles_left);	// cycles_left is negative here, so substraction is correct
				cycles_left = cpu_throttle_cycles;
				break;
			case Z80_EVENT_SHUTDOWN:
				CPMPRG_STOP(-1, "%s(): Z80 SHUTDOWN event shouldn't be received from CPU emulation", __func__);
				break;
			case 0:
				CPMPRG_STOP(-1, "%s(): no-event cannot be passed to the event handler", __func__);
				break;
			default:
				CPMPRG_STOP(-1, "%s(): unknown CPU emulation event: %d", __func__, z80ev.event);
				break;
		}
	} while (LIKELY(z80ev.event != Z80_EVENT_SHUTDOWN));
	return termination_code;
}


void cpmprg_z80_reset ( void )
{
	z80ex_reset();
	Z80_PC = 0x100;
	Z80_SP = BDOS_ENTRY_ADDR;
	z80ev.user_mem_first_byte = 8;
	z80ev.user_mem_last_byte = BDOS_ENTRY_ADDR - 1;
}


// Signals Z80 emulation loop in cpmprg_z80_execute() for being "shutdown", ie exit.
// cpmprg_z80_execute() will return the integer given to this function.
// Also, the variadic parameter can be used to pass a printf' style syntax
// to describe the situation. It's placed into a buffer, which can be print'ed
// or whatever by the caller when function cpmprg_z80_execute() returned.
// Actually it's possible to pass NULL as the format, for denoting no information.

int CPMPRG_STOP ( int code, const char *format, ... )
{
	termination_code = code;
	z80ev.event = Z80_EVENT_SHUTDOWN;
	if (format && *format) {
		va_list args;
		va_start(args, format);
		vsnprintf(termination_msg, sizeof termination_msg, format, args);
		va_end(args);
	} else
		termination_msg[0] = '\0';
	return code;
}


void cpm_memory_initialize ( void )
{
	memset(memory, 0, sizeof memory);
	/* create jump table for CBIOS emulation, actually they're CPU traps, and RET! */
	for (int a = 0; a < CBIOS_ENTRIES; a++) {
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
}


int cpmprg_prepare_psp ( int argc, char **argv )
{
	memset(memory + 8, 0, 0x100 - 8);
	memset(memory + 0x5C + 1, 32, 11);
	memset(memory + 0x6C + 1, 32, 11);
	for (int a = 0; a < argc; a++) {
		if (a <= 1)
			write_filename_to_fcb(a == 0 ? 0x5C : 0x6C, argv[a]);
		if (memory[0x81])
			strcat((char*)memory + 0x81, " ");
		strcat((char*)memory + 0x81, argv[a]);
		if (strlen((char*)memory + 0x81) > 0x7F)
			return CPMPRG_STOP(1, "Too long command line for the CP/M program");
	}
	memory[0x80] = strlen((char*)memory + 0x81);
	return 0;
}



int cpmprg_load_and_execute ( const char *hostospath, int argc, char **argv )
{
	cpm_memory_initialize();
	int fd = open(hostospath, O_RDONLY | O_BINARY);
	if (fd < 0)
		return CPMPRG_STOP(1, "Cannot open program file: %s: %s", hostospath, strerror(errno));
	int size = read(fd, memory + 0x100, BDOS_ENTRY_ADDR - 0x100);
	int err = errno;
	close(fd);
	if (size < 0)
		return CPMPRG_STOP(1, "I/O error while loading program: %s", strerror(err));
	if (size < 10)
		return CPMPRG_STOP(1, "Too short CP/M program");
	if (size > BDOS_ENTRY_ADDR - 0x100 - 1)
		return CPMPRG_STOP(1, "Too large CP/M program file");
	if ((size = cpmprg_prepare_psp(argc, argv)))
		return size;
	cpmprg_z80_reset();
	return cpmprg_z80_execute();
}




#if 0

/* Intitialize CP/M emulation */
static int cpm_init ( int argc, char **argv )
{
	FILE *f;
	int a;
	stop_execution(0, NULL);
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
	DEBUG("*** Starting program: %s with parameters %s\n", argv[1], (char*)memory + 0x81);
	return 0;
}

#endif
