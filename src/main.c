#define _CRT_SECURE_NO_WARNINGS
#include "common.h"
#include "repo.h"
#include "commit.h"
#include "branch.h"
#include "diff.h"
#include "status.h"

static void print_help(void) {
    printf("%sMyGit%s - Simplified Version Control System\n\n", COLOR_GREEN, COLOR_RESET);
    printf("Usage: mygit <command> [options]\n\n");
    printf("Available commands:\n\n");

    printf("  %sinit%s\t\t\tInitialize empty repository\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %sadd%s <path>\t\tAdd files to staging area\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %sremove%s <file>\t\tMark file for deletion\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %scommit%s <message>\t\tCreate new commit\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %slog%s [options]\t\tShow commit history\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %sdiff%s [commits]\t\tShow differences\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %sstatus%s\t\t\tShow working tree status\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %scheckout%s <ref>\t\tSwitch branches or restore files\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %sbranch%s [name]\t\tCreate or list branches\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s--help%s\t\t\tDisplay this help\n\n", COLOR_YELLOW, COLOR_RESET);

    printf("Examples:\n");
    printf("  mygit init\n");
    printf("  mygit add main.c\n");
    printf("  mygit commit \"Initial commit\"\n");
    printf("  mygit log -n 5\n");
    printf("  mygit branch feature-x\n");
    printf("  mygit checkout feature-x\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    const char* command = argv[1];
    int cmd_argc = argc - 2;
    char** cmd_argv = argv + 2;

    if (strcmp(command, "init") == 0) {
        return cmd_init(cmd_argc, cmd_argv);
    }
    else if (strcmp(command, "add") == 0) {
        return cmd_add(cmd_argc, cmd_argv);
    }
    else if (strcmp(command, "remove") == 0 || strcmp(command, "rm") == 0) {
        return cmd_remove(cmd_argc, cmd_argv);
    }
    else if (strcmp(command, "commit") == 0) {
        return cmd_commit(cmd_argc, cmd_argv);
    }
    else if (strcmp(command, "log") == 0) {
        return cmd_log(cmd_argc, cmd_argv);
    }
    else if (strcmp(command, "diff") == 0) {
        return cmd_diff(cmd_argc, cmd_argv);
    }
    else if (strcmp(command, "status") == 0) {
        return cmd_status(cmd_argc, cmd_argv);
    }
    else if (strcmp(command, "checkout") == 0) {
        return cmd_checkout(cmd_argc, cmd_argv);
    }
    else if (strcmp(command, "branch") == 0) {
        return cmd_branch(cmd_argc, cmd_argv);
    }
    else {
        fprintf(stderr, "%sError:%s Unknown command '%s'\n",
            COLOR_RED, COLOR_RESET, command);
        fprintf(stderr, "Use 'mygit --help' for usage information.\n");
        return 1;
    }

    return 0;
}