/** vim: set expandtab softtabstop=4 tabstop=4 shiftwidth=4: */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#define DIRSEPSTR "\\"
#else
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/utsname.h>
#define DIRSEPSTR "/"
#endif


#define DRIVES 16


static struct {
    char host_path[PATH_MAX];
    int read_only;
} cpm_drives[DRIVES];
static int current_drive = 0;


static int tokenizer ( char *line, const int max_tokens, char **tokens, const char *comment_signs )
{
    if (max_tokens < 1)
        return 0;
    char *token;
    int num = 0;
    do {
        token = strtok(line, "\r\n\t ");
        if (token) {
            printf("token=[%s]\n", token);
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
    if (drive >= DRIVES)
        return -2;
    if (drive >= 0 && drive_only && strlen(s) != 2)
        return -1;
    return drive;
}


static int show_drive_assignment ( int drive )
{
    if (drive >= 0 && drive < DRIVES && cpm_drives[drive].host_path[0]) {
        printf("%c: -> %s [%s]\n", drive + 'A', cpm_drives[drive].host_path, cpm_drives[drive].read_only ? "RO" : "RW");
        return 0;
    }
    return 1;
}



static int drive_logon ( int drive, const char *host_path )
{
    if (drive < 0 || drive >= DRIVES) {
        fprintf(stderr, "Invalid drive: #%d\n", drive);
        return -1;
    }
    if (cpm_drives[drive].host_path[0]) {
        if (chdir(cpm_drives[drive].host_path)) {
            perror("chdir()");
        }
    }
    char resolved_path[PATH_MAX];
    if (
#ifdef _WIN32
        GetFullPathName(host_path, PATH_MAX, resolved_path, NULL)
#else
        realpath(host_path, resolved_path)
#endif
        && !chdir(resolved_path))
    {
        strcpy(cpm_drives[drive].host_path, resolved_path);
        strcat(cpm_drives[drive].host_path, DIRSEPSTR);
        cpm_drives[drive].read_only = 0;
        show_drive_assignment(drive);
        //printf("Drive %c: logged on to %s\n", drive + 'A', resolved_path);
        return 0;
    }
    perror("realpath()");
    return -1;
}



static int shellcmd_cd ( const char **tokens, int num_tokens, const char *orig_line )
{
    if (num_tokens == 0) {
        for (int a = 0; a < DRIVES; a++)
            show_drive_assignment(a);
//            if (cpm_drives[a].host_path[0])
//                printf("%c: -> %s [%s]\n", a + 'A', cpm_drives[a].host_path, cpm_drives[a].read_only ? "RO" : "RW");
        return 0;
    }
    int drive = resolve_drive_token(tokens[0], 1);
    printf("Drive: %d num_tokens = %d\n", drive, num_tokens);
    if (drive >= 0 && num_tokens == 2)
        return drive_logon(drive, tokens[1]);
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
    const char *help;
};
#define MAKE_COMMAND(cmd,callback,helpy)
static int shellcmd_help ( const char **tokens, int num_tokens, const char *orig_line );

static const struct shell_command_list_entry_st shell_commands[] = {
    {"HELP", shellcmd_help, "Help command ...."},
    {"CD", shellcmd_cd, "Host-OS level directory change/query, CP/M drive assignment to host-OS directory\n"
"CD                        List drive assignments\n"
"CD A:                     Show drive assignemnt only a specific drive, A: in this case\n"
"CD A: some-host-os-path   Assign/change a host-OS directory to a CP/M drive (A: here)\n"
"CD some-host-os-path      Change a host-OS directory assigned to the current CP/M drive\n"
"CD B: -                   Delete assignment for a given CP/M drive (\n"
"This command allows you to deal with emulation-specific aspects of the CP/M\n"
"file system. In reCPM, the original CP/M filesystem is not emulated at block\n"
"I/O level, rather then the host-OS (the OS runs thish emulator) file system\n"
"is used at FCB level only. CP/M 2.x does not know about directioes at all.\n"
"Though, it know the notion of drives like A:. In fact, this is the source\n"
"of drive names even in Windows till the very current day. To bridge the\n"
"the difference, we can assign a given host-OS directory to a given CP/M drive.\n"
},
    {"DIR", shellcmd_dir, "Directory"},
    {NULL, NULL, NULL}
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
    if (tokens[0]) {
        const struct shell_command_list_entry_st *cmd = search_builtin_command(tokens[0]);
        if (cmd) {
            printf("%s\n", cmd->help ? cmd->help : "EMPTY HELP PAGE :(");
            return 0;
        } else {
            printf("No help on unknown command '%s'. Type simply 'help' to get summary/list of commands\n", tokens[0]);
            return 1;
        }
    }
    printf("Hi dude, I am your help ;-P\n");
    //This page will present you the summary of built-in reSHELL commands (see below).
    //To get a more detailed help on a specific command, use the command name as a parameter of the help command.

    const struct shell_command_list_entry_st *p = shell_commands;
    while (p->command_name) {

        p++;
    }



    for (int a = 0; tokens[a]; a++) {
        printf("%s\n", tokens[a]);
    }
    return 0;
}




static int shell_process_command ( const char *original_line )
{
    if (!original_line || !*original_line) {
        puts("Empty command ...");
        return 0;
    }
    if (strlen(original_line) > 255) {
        puts("Too long command");
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
    for (int a = 0; a < tk; a++)
        printf("tok-parse-%d=\"%s\"\n", a, tokens[a]);
    // check, if command is chnage drive, like "A:"
    if (tk == 1 && tokens[0][strlen(tokens[0]) -1 ] == ':') {
         printf("Select drive?\n");
    }
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


static char* shell_readline_interactive ( const char *prompta )
{
    char prompt[3] = { current_drive + 'A', '>', 0 };
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






int shell_main ( void )
{
#ifndef _WIN32
    if (isatty(1) == 1) {
        char *tty = ttyname(1);
        printf("Running on console: %s\n", tty);
        puts("\033]0;reCPM ~ [SHELL>A]\007");
    }
#else
    //FreeConsole();
    AllocConsole();
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    SetConsoleTitle("CP/M console");
#endif
    for (int a = 0; a < DRIVES; a++) {
        cpm_drives[a].host_path[0] = '\0';
        cpm_drives[a].read_only = 0;
    }
    show_version();
    //if (!getcwd(cpm_drives[0].host_path, sizeof cpm_drives[0].host_path)) {
    //    perror("Querying current host-OS directory");
    //    return 1;
    //}
    if (drive_logon(0, ".")) {
        fprintf(stderr, "Cannot logon A: to current host-OS directory\n");
        return 1;
    }
    puts("reSHELL running. For help on shell, commands and on help itself, use command help.");
    for (;;) {
        //char *line = readline("A>");
        char *line = shell_readline_interactive("A>");
        if (!line) {
            puts("");
            break;
        }
        if (strlen(line) > 255) {
            fprintf(stderr, "Too long line\n");
        } else {
            printf("[%s]\n", line);
            shell_process_command(line);
#if 0
            char *tokens[10];
            int tk = parse(line, 9, tokens);
            for (int a = 0; a < tk; a++)
                printf("tok-parse-%d=\"%s\"\n", a, tokens[a]);
            if (tk == 1 && tokens[0][strlen(tokens[0]) -1 ] == ':') {
                printf("Select drive?\n");
            }
#endif
        }
        free(line);
    }
    return 0;
}
