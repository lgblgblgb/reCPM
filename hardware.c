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

#include <hardware.h>




Uint8 memory[0x10000];
Z80EX_CONTEXT z80ex;
struct z80ev_st z80ev;



static inline int z80ex_ed_cb(Z80EX_BYTE opcode) {
	z80ev.event = Z80_EVENT_ED_TRAP;
	z80ev.fault_data = opcode;
	z80ev.fault_pc = (Z80_PC - 2) & 0xFFFF;
	z80ev.fault_addr = z80ev.fault_pc;
	return 0;
}

static INLINE Z80EX_BYTE z80ex_mread_cb(Z80EX_WORD addr, int m1_state) {
	return memory[addr];
}

static inline void z80ex_mwrite_cb(Z80EX_WORD addr, Z80EX_BYTE value) {
	if (addr > z80ev.user_mem_last_byte || addr < z80ev.user_mem_first_byte) {
		z80ev.event = Z80_EVENT_MPROTECT;
		z80ev.fault_data = value;
		z80ev.fault_pc = Z80_PC;
		z80ev.fault_addr = addr;
	} else
		memory[addr] = value;
#if 0
	if (addr >= BDOS_ENTRY_ADDR) {
		DEBUG("CPM: FATAL: someone tried to write system area at address %04Xh PC = %04Xh\n", addr, Z80_PC);
		exit(1);
	}
	if (addr < 0x10) {
		DEBUG("CPM: warning, someone has just written low-area memory at address %02Xh PC = %04Xh\n", addr, Z80_PC);
	}
#endif
}

static inline Z80EX_BYTE z80ex_pread_cb(Z80EX_WORD port16) {
//	DEBUG("CPM: warning, someone tried to read I/O port %04Xh at PC = %04Xh\n", port16, Z80_PC);
	return 0xFF;
}

static inline void z80ex_pwrite_cb(Z80EX_WORD port16, Z80EX_BYTE value) {
//	DEBUG("CPM: warning, someone tried to write I/O port %04Xh at PC = %04Xh\n", port16, Z80_PC);
}

static inline Z80EX_BYTE z80ex_intread_cb( void ) {
	return 0xFF;
}

static inline void z80ex_reti_cb ( void ) {
}

#include "z80ex/z80ex.c"
#include "z80ex/z80ex_dasm.c"

static Z80EX_BYTE disasm_mreader ( Z80EX_WORD addr )
{
	return z80ex_mread_cb(addr, 0);
}

int z80_custom_disasm ( int addr, char *buf, int buf_size )
{
	int t1, t2;
	char o_head[256];
	char o_dasm[256];
	int oplen = z80ex_dasm(o_dasm, sizeof o_dasm, 0, &t1, &t2, disasm_mreader, addr & 0xFFFF);
	for (int a = 0; a < oplen; a++)
		sprintf(o_head + a * 3, "%02X ", disasm_mreader(addr + a));
	snprintf(buf, buf_size, "%04X %-12s %s", addr, o_head, o_dasm);
	return oplen;
}
