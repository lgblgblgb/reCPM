/* This is a MINIMAL CP/M emulator. That is, it emulates a Z80 (though 8080 would be
   enough for most CP/M software), the BDOS and the CBIOS as well, and just expects
   a CP/M program to load and run. It was written *ONLY FOR* running some ancient
   tools, namely M80 assembler and L80 linker (they don't seem to have modern versions
   for non-CP/M OSes). The emulator has BDOS/CBIOS functions implemented and in a way
   that is enough for these tools! Maybe this can change in the future, but this is the
   current situation! Stderr should be redirected, as those are DEBUG messages.

   Copyright (C)2018 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>

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

#ifndef __XCPM_COMMON_H_INCLUDED
#define __XCPM_COMMON_H_INCLUDED

#include <stdint.h>

typedef uint8_t         Uint8;
typedef int8_t          Sint8;
typedef uint16_t        Uint16;
typedef int16_t         Sint16;
typedef uint32_t        Uint32;
typedef int32_t         Sint32;

#ifndef _WIN32
#define O_BINARY 0
#endif

#ifdef __GNUC__
#	define LIKELY(__x__)	__builtin_expect(!!(__x__), 1)
#	define UNLIKELY(__x__)	__builtin_expect(!!(__x__), 0)
#	ifdef DO_NOT_FORCE_INLINE
#		define INLINE	inline
#	else
#		define INLINE	__attribute__ ((__always_inline__)) inline
#	endif
#else
#	warning "No GCC extensions are used (__GNUC__ is not defined) which would help to improve performance."
#	define LIKELY(__x__)	(__x__)
#	define UNLIKELY(__x__)	(__x__)
#	define INLINE		inline
#endif

#define DEBUG(...) fprintf(stderr, __VA_ARGS__)

#endif
