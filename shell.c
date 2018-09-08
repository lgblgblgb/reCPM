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


static int shell_exit_request;



extern const char help_md[];










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
			printf("Got joker search: \"%s\" [size=%d]\n", topic, topic_size);
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
						items++;
						while (p <= nl)
							putchar(*p++);
						while (*p && *p == '\n')
							p++;
						printf("    ");
						while (*p && *p != '\n')
							putchar(*p++);
						putchar('\n');
					}
				}
			} while (p);
			if (!items) {
				printf("No topics found matching your pattern '%s'\n", topic);
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
			while (*p && !(p[0] == '\n' && p[1] == '#' && p[2] != '#'))
				putchar(*p++);
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



static void show_version ( void )
{
#ifndef _WIN32
	struct utsname uts;
	uname(&uts);
	printf("reCPM " DATECODE " on %s %s %s \"%s\"\n",
		uts.sysname, uts.release, uts.machine, uts.nodename
	);
#else
	DWORD winver = GetVersion();
	char nodename[100];
	DWORD nodesize = sizeof(nodename) - 1;
	printf("reCPM v%s on Windows %d.%d #%d %s \"%s\"\n",
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
	printf("CP/M desired compatibility level v2.2\n");
}



int console_title ( const char *name )
{
#ifdef _WIN32
	char title_str[strlen(name) + 40];
	sprintf(title_str,"reCPM ~ %s", name);
	SetConsoleTitle(title_str);
#else
	if (isatty(1) == 1) {
		printf("\033]0;reCPM ~ %s\007", name);
	}
#endif
	return 0;
}



int console_init ( void )
{
#ifdef _WIN32
	AllocConsole();
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
	console_title("????");
#else
	if (isatty(0) == 1) {
		char *tty = ttyname(0);
		printf("Running on console for STDIN: %s\n", tty);
	}
	if (isatty(1) == 1) {
		char *tty = ttyname(1);
		console_title("????");
		printf("Running on console for STDOUT: %s\n", tty);
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
