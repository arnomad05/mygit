#define _CRT_SECURE_NO_WARNINGS
#include "status.h"
#include "repo.h"
#include "file_ops.h"
#include "hash.h"
#include "string_utils.h"

int cmd_status(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Repository* repo = repo_open(".");
    if (!repo) {
        fprintf(stderr, "%sError:%s Not a MyGit repository.\n",
            COLOR_RED, COLOR_RESET);
        return 1;
    }

    if (repo->branch_manager->is_detached) {
        printf("HEAD detached at %s\n",
            sha1_to_string(&repo->branch_manager->detached_hash));
    }
    else {
        printf("On branch %s\n", repo->branch_manager->current_branch);
    }

    printf("\n");

    if (repo->staged_count > 0) {
        printf("Changes to be committed:\n");
        printf("  (use \"mygit remove --cached <file>...\" to unstage)\n\n");

        for (int i = 0; i < repo->staged_count; i++) {
            const char* action = "modified";
            if (repo->staged_files[i].action == FILE_ADDED) {
                action = "new file";
            }
            else if (repo->staged_files[i].action == FILE_DELETED) {
                action = "deleted";
            }

            printf("        %s: %s\n", action, repo->staged_files[i].filename);
        }
        printf("\n");
    }

    if (repo->staged_count == 0) {
        printf("nothing to commit, working tree clean\n");
    }

    repo_close(repo);
    return 0;
}