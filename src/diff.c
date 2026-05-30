#define _CRT_SECURE_NO_WARNINGS
#include "diff.h"
#include "repo.h"
#include "commit.h"
#include "file_ops.h"
#include "string_utils.h"

int cmd_log(int argc, char* argv[]) {
    Repository* repo = repo_open(".");
    if (!repo) {
        fprintf(stderr, "Error: Not a MyGit repository.\n");
        return 1;
    }

    int max_commits = -1;
    const char* start_ref = NULL;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            max_commits = atoi(argv[i + 1]);
            i++;
        }
        else if (argv[i][0] != '-') {
            start_ref = argv[i];
        }
    }

    CommitNode* commit = repo->head_commit;
    if (start_ref) {
        commit = repo_resolve_commit(repo, start_ref);
        if (!commit) {
            fprintf(stderr, "Error: Unknown commit: %s\n", start_ref);
            repo_close(repo);
            return 1;
        }
    }

    int count = 0;

    
    while (commit && (max_commits == -1 || count < max_commits)) {
        char* hash_str = sha1_to_string(&commit->hash);
        printf("commit %s\n", hash_str);
        free(hash_str);

        printf("Date:   %s", ctime(&commit->timestamp));
        printf("\n    %s\n\n", commit->message);

        count++;
        commit = commit->prev;
    }

    repo_close(repo);
    return 0;
}

int cmd_diff(int argc, char* argv[]) {
    Repository* repo = repo_open(".");
    if (!repo) {
        fprintf(stderr, "%sError:%s Not a MyGit repository.\n",
            COLOR_RED, COLOR_RESET);
        return 1;
    }

    if (argc < 1) {
        fprintf(stderr, "%sError:%s Specify commit to compare.\n",
            COLOR_RED, COLOR_RESET);
        fprintf(stderr, "Usage: mygit diff <commit>\n");
        repo_close(repo);
        return 1;
    }

    CommitNode* commit = repo_resolve_commit(repo, argv[0]);
    if (!commit) {
        fprintf(stderr, "%sError:%s Unknown commit: %s\n",
            COLOR_RED, COLOR_RESET, argv[0]);
        repo_close(repo);
        return 1;
    }

    printf("Comparing with commit %s\n\n", sha1_to_string(&commit->hash));

    for (int i = 0; i < commit->file_count; i++) {
        FileRecord* head_file = commit_find_file(repo->head_commit,
            commit->files[i].filename);

        if (!head_file) {
            printf("%sDeleted:%s %s\n", COLOR_RED, COLOR_RESET,
                commit->files[i].filename);
        }
        else if (!sha1_compare(&commit->files[i].hash, &head_file->hash)) {
            printf("%sModified:%s %s\n", COLOR_YELLOW, COLOR_RESET,
                commit->files[i].filename);
        }
    }

    repo_close(repo);
    return 0;
}