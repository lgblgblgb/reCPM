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


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/utsname.h>
#endif

#include "common.h"
#include "shell.h"
#include "cpm.h"
#include "hardware.h"


static int shell_exit_request;
int stdin_is_tty = 0;
int stdout_is_tty = 0;

extern const char help_md[];




static void show_version ( void )
{
#ifndef _WIN32
	struct utsname uts;
	uname(&uts);
	printf("re-CP/M " DATECODE " running on %s %s %s \"%s\"\n",
		uts.sysname, uts.release, uts.machine, uts.nodename
	);
#else
	DWORD winver = GetVersion();
	char nodename[100];
	DWORD nodesize = sizeof(nodename) - 1;
	printf("re-CP/M v%s running on Windows %d.%d #%d %s \"%s\"\n",
		DATECODE,
		LOBYTE(LOWORD(winver)),
		HIBYTE(LOWORD(winver)),
		(winver < 0x80000000) ? HIWORD(winver) : 0,
#ifdef _WIN64
		"x86_64",
#else
		"x86_32",
#endif
		GetComputerName(nodename, &nodesize) ? "unknown-host" : nodename
	);
	//IsWow64Process
#endif
	printf(
		"(C)2016,2018 LGB Gabor Lenart - https://github.com/lgblgblgb/reCPM\n"
		"This is a GNU/GPL software, type HELP LICENSE for more information.\n"
		"CP/M desired compatibility level v2.2\n"
	);
}


static void strcapitalize ( char *dst, const char *src, int bufsize )
{
	while (bufsize > 0 && *src) {
		*dst++ = toupper(*src++);
		bufsize--;
	}
	*dst = '\0';
}


static int tokenizer ( char *line, const int max_tokens, char **tokens, const char *comment_signs )
{
	if (max_tokens < 1)
		return 0;
	char *token;
	int num = 0;
	do {
		token = strtok(line, "\r\n\t ");
		if (token) {
			//printf("token=[%s]\n", token);
			if (strchr(comment_signs, token[0]))
				token = NULL;
			else {
				line = NULL;
				if (num < max_tokens)
					tokens[num++] = token;
				else
					return -1;
			}
		}
	} while (token);
	tokens[num] = NULL;
	return num;
}


static int resolve_drive_token ( const char *s, int drive_only )
{
	int drive = -1;
	if (s && s[0] && s[1] == ':') {
		if (s[0] >= 'A' && s[0] <= 'Z')
			drive = s[0] - 'A';
		else if (s[0] >= 'a' && s[0] <= 'z')
			drive = s[0] - 'a';
	}
	if (drive >= CPM_DRIVES_MAX)
		 return -2;
	if (drive >= 0 && drive_only && strlen(s) != 2)
		return -1;
	return drive;
}


static int show_drive_assignment ( int drive )
{
	if (drive >= 0 && drive < CPM_DRIVES_MAX && cpm_drives[drive].host_path[0]) {
		printf("%c: -> %s [%s]\n", drive + 'A', cpm_drives[drive].host_path, cpm_drives[drive].read_only ? "RO" : "RW");
		return 0;
	}
	return 1;
}


static void show_all_drive_assignments ( void )
{
	for (int a = 0; a < CPM_DRIVES_MAX; a++)
		show_drive_assignment(a);
}


static int print_cpm_errmsg ( int code )
{
	if (code)
		fprintf(stderr, "CP/M error: %s\n", cpm_last_errmsg[0] ? cpm_last_errmsg : "unspecified error, no error message");
	cpm_last_errmsg[0] = '\0';
	return code;
}



static int shellcmd_exit ( const char **tokens, int num_tokens, const char *orig_line )
{
	shell_exit_request = num_tokens ? abs(atoi(tokens[0])) : 0;
	return 0;
}

static int shellcmd_cd ( const char **tokens, int num_tokens, const char *orig_line )
{
	if (num_tokens == 0) {
		show_all_drive_assignments();
		return 0;
	}
	int drive = resolve_drive_token(tokens[0], 1);
	printf("Drive: %d num_tokens = %d\n", drive, num_tokens);
	if (drive >= 0 && num_tokens == 2)
		return print_cpm_errmsg(cpm_drive_logon(drive, tokens[1]));
	return 1;
}


static int shellcmd_dir ( const char **tokens, int num_tokens, const char *orig_line )
{
	if (num_tokens == 0) {
		printf("Directory of current drive, all files\n");
		return 0;
	}
	int drive = resolve_drive_token(tokens[0], 0);
	printf("Drive = %d\n", drive);
	return 1;
}


static int string2hexword ( const char *s )
{
	char *endptr;
	long int addr = strtol(s, &endptr, 16);
	return (*endptr || addr < 0 || addr > 0xFFFF) ? -1 : addr & 0xFFFF;
}




static int shellcmd_mdump ( const char **tokens, int num_tokens, const char *orig_line )
{
	static int addr = 0x100;
	if (num_tokens) {
		int new_addr = string2hexword(tokens[0]);
		if (new_addr < 0) {
			printf("Invalid hex address to dump from\n");
			return 1;
		} else
			addr = new_addr;
	}
	char s[100];
	memset(s, 0, sizeof s);
	for (int line = 0; line < 16; line++) {
#		define ZBYTE(a) memory[(addr + (a)) & 0xFFFF]
#		define ZCHAR(a) ((ZBYTE(a) >= 0x20 && ZBYTE(a) < 127) ? ZBYTE(a) : '.')
		printf("%04X  %02X %02X %02X %02X %02X %02X %02X %02X-%02X %02X %02X %02X %02X %02X %02X %02X  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
			addr,
			ZBYTE(0), ZBYTE(1), ZBYTE(2), ZBYTE(3), ZBYTE(4), ZBYTE(5), ZBYTE(6), ZBYTE(7),
			ZBYTE(8), ZBYTE(9), ZBYTE(10), ZBYTE(11), ZBYTE(12), ZBYTE(13), ZBYTE(14), ZBYTE(15),
			ZCHAR(0), ZCHAR(1), ZCHAR(2), ZCHAR(3), ZCHAR(4), ZCHAR(5), ZCHAR(6), ZCHAR(7),
			ZCHAR(8), ZCHAR(9), ZCHAR(10), ZCHAR(11), ZCHAR(12), ZCHAR(13), ZCHAR(15), ZCHAR(15)
		);
#		undef ZCHAR
#		undef ZBYTE
		addr = (addr + 0x10) & 0xFFFF;
	}
	return 0;
}



static int shellcmd_dasm ( const char **tokens, int num_tokens, const char *orig_line )
{
	static int addr = 0x100;
	if (num_tokens) {
		int new_addr = string2hexword(tokens[0]);
		if (new_addr < 0) {
			printf("Invalid hex address to dump from\n");
			return 1;
		} else
			addr = new_addr;
	}
	int lines = 20;
	while (lines--) {
		char buf[256];
		addr = (addr + z80_custom_disasm(addr, buf, sizeof buf)) & 0xFFFF;
		printf("%s\n", buf);
	}
	return 0;
}


static int shellcmd_ver ( const char **tokens, int num_tokens, const char *orig_line )
{
	show_version();
	return 0;
}




struct shell_command_list_entry_st {
	const char *command_name;
	int (*exec)(const char **, int, const char *);
};

static int shellcmd_help ( const char **tokens, int num_tokens, const char *orig_line );

static const struct shell_command_list_entry_st shell_commands[] = {
	{ "EXIT",	shellcmd_exit	},
	{ "HELP",	shellcmd_help	},
	{ "CD",		shellcmd_cd	},
	{ "DIR",	shellcmd_dir	},
	{ "MDUMP",	shellcmd_mdump	},
	{ "DASM",	shellcmd_dasm	},
//	{ "GO",		shellcmd_go	},
	{ "VER",	shellcmd_ver	},
	{ NULL,		NULL		}
};



static const struct shell_command_list_entry_st *search_builtin_command ( const char *command_name )
{
	const struct shell_command_list_entry_st *p = shell_commands;
	while (p->command_name)
		if (!strcasecmp(p->command_name, command_name))
			return p;
		else
			p++;
	return NULL;
}


static int shellcmd_help ( const char **tokens, int num_tokens, const char *orig_line )
{
	if (num_tokens) {
		if (tokens[0][strlen(tokens[0]) - 1] == '?') {
			const char *p = help_md;
			char topic[32];
			int topic_size = strlen(tokens[0]) - 1;
			int items = 0;
			if (topic_size >= sizeof(topic))
				topic_size = sizeof(topic) - 1;
			strcapitalize(topic, tokens[0], topic_size);
			//printf("Got joker search: \"%s\" [size=%d]\n", topic, topic_size);
			do {
				p = strstr(p, "\n#");
				if (p && p[2] != '\n' && p[2] != '#' && p[2]) {
					p += 2;
					const char *nl = strchr(p, '\n');
					if (topic_size) {
						const char *t = strstr(p, topic);
						if (!t || t >= nl)
							nl = NULL;
					}
					if (nl) {
						int align = 10;
						items++;
						while (p < nl) {
							putchar(*p++);
							align--;
						}
						while (*p && *p == '\n')
							p++;
						while (align-- > 0)
							putchar(' ');
						while (*p && *p != '\n')
							putchar(*p++);
						putchar('\n');
					}
				}
			} while (p);
			if (!items) {
				printf("No topic found matching your pattern '%s'\n", topic);
				return 1;
			}
			return 0;

		} else {
			char topic[32];
			topic[0] = '\n';
			topic[1] = '#';
			strcapitalize(topic + 2, tokens[0], sizeof(topic) - 4);
			//printf("SIZE: %d\n", strlen(topic));
			strcat(topic, "\n");
			//printf("SIZE: %d\n", strlen(topic));
			const char *p = strstr(help_md, topic);
			if (!p) {
				printf("HELP: no such help page to show: '%s'\nTry command HELP %s? to search matching pages, it may helps\n", tokens[0], tokens[0]);
				return 1;
			}
			printf("=== reCPM manual >>> ");
			p += 2;
			int lines = 0;
			while (*p && !(p[0] == '\n' && p[1] == '#' && p[2] != '#')) {
				putchar(*p);
				if (*p == '\n') {
					lines++;
					if (lines == 22) {
						if (stdin_is_tty && stdout_is_tty)
							printf("---- NEW PAGE ----\n");	// TODO: implement pager.
						lines = 0;
					}
				}
				p++;
			}
			if (p[-1] != '\n')
				putchar('\n');
			return 0;
		}
	}
	puts(
		"You need to specifiy what you want to get help on.\n"
		"For more information and examples on help, use command (help on help ...): HELP HELP\n"
		"Available shell commands:"
	);
	const struct shell_command_list_entry_st *p = shell_commands;
	while (p->command_name) {
		printf(" %s", p->command_name);
		p++;
	}
	putchar('\n');
	return 0;
}




static int shell_process_command ( const char *original_line )
{
	if (!original_line || !*original_line) {
		// puts("Empty command ...");
		return 0;
	}
	if (strlen(original_line) > 255) {
		puts("Too long command line");
		return 1;
	}
	char line[256];
	strcpy(line, original_line);
	char *tokens[10];
	int tk = tokenizer(line, 9, tokens, "#");
	if (tk < 0) {
		puts("Too many tokens");
		return 1;
	}
	if (tk == 0) {
		return 0;
	}
#if 0
	for (int a = 0; a < tk; a++)
		printf("tok-parse-%d=\"%s\"\n", a, tokens[a]);
#endif
	// check, if command is chnage drive, like "A:"
	if (tk == 1) {
		int drive = resolve_drive_token(tokens[0], 1);
		if (drive >= 0)
			return print_cpm_errmsg(cpm_select_drive(drive));
	}
	int drive;
	if (tk == 1 && (drive = resolve_drive_token(tokens[0], 1) >= 0))
		return print_cpm_errmsg(cpm_select_drive(drive));
	// check, if command is a built-in command
	const struct shell_command_list_entry_st *cmd = search_builtin_command(tokens[0]);
	if (!cmd) {
		printf("Unknown command %s\n", tokens[0]);
		return -1;
	}
	if (!cmd->exec) {
		printf("Built-in commmand %s cannot be executed directly.\n", tokens[0]);
		return -1;
	}
	cmd->exec((const char**)tokens + 1, tk - 1, original_line);
	return 0;
}


static char* shell_readline_interactive ( void )
{
	char prompt[3] = { current_cpm_drive + 'A', '>', 0 };
#ifdef _WIN32
	char *buffer = malloc(260);
	if (!buffer)
		return NULL;
	printf("%s", prompt);
	if (fgets(buffer, 257, stdin))
		return buffer;
	else {
		free(buffer);
		return NULL;
	}
#else
	char *buffer = readline(prompt);
	if (buffer && *buffer)
		add_history(buffer);
	return buffer;
#endif
}



int console_title ( const char *name )
{
	char title_str[strlen(name) + 40];
	sprintf(title_str,"re-CP/M ~ %s", name);
#ifdef _WIN32
	SetConsoleTitle(title_str);
#else
	if (stdout_is_tty) {
		printf("\033]0;%s\007", title_str);
	}
#endif
	return 0;
}



int console_init ( void )
{
#ifdef _WIN32
	//FreeConsole();
	AllocConsole();
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
	stdin_is_tty = 1;
	stdout_is_tty = 1;
#else
	if (isatty(0) == 1) {
		char *tty = ttyname(0);
		DEBUG("Running on console for STDIN: %s\n", tty);
		stdin_is_tty = 1;
	}
	if (isatty(1) == 1) {
		char *tty = ttyname(1);
		DEBUG("Running on console for STDOUT: %s\n", tty);
		stdout_is_tty = 1;
	}
#endif
	return 0;
}




int shell_main ( void )
{
	show_version();
	if (cpm_drive_logon(0, ".")) {
		print_cpm_errmsg(-1);
		fprintf(stderr, "Cannot logon A: to current host-OS directory\n");
		return 1;
	}
	show_all_drive_assignments();
	puts("reSHELL running. Use command 'HELP' and 'HELP HELP' to get more information.");
	shell_exit_request = -1;
	do {
		char *line = shell_readline_interactive();
		if (!line) {
			puts("exit");
			shell_exit_request = 0;
			break;
		}
		if (strlen(line) > 255) {
			fprintf(stderr, "Too long line\n");
		} else {
			//printf("[%s]\n", line);
			shell_process_command(line);
		}
		free(line);
	} while (shell_exit_request < 0);
	return shell_exit_request;
}
