#ifndef REPO_H
#define REPO_H

#include "common.h"
#include "hash.h"
#include "commit.h"
#include "branch.h"

typedef struct {
    char repo_path[MAX_PATH_LEN];
    char git_dir[MAX_PATH_LEN];

    CommitNode* head_commit;
    BranchManager* branch_manager;

    FileRecord* staged_files;
    int staged_count;
    int staged_capacity;

    char** tracked_files;
    int tracked_count;
    int tracked_capacity;

    bool is_initialized;
} Repository;

Repository* repo_init(const char* path);
Repository* repo_open(const char* path);
void repo_close(Repository* repo);
bool repo_is_valid(const char* path);

bool repo_stage_file(Repository* repo, const char* filename);
bool repo_unstage_file(Repository* repo, const char* filename);
bool repo_stage_remove(Repository* repo, const char* filename);
void repo_clear_staging(Repository* repo);

CommitNode* repo_get_head_commit(Repository* repo);
bool repo_add_commit(Repository* repo, CommitNode* commit);
CommitNode* repo_find_commit(Repository* repo, const SHA1Hash* hash);
CommitNode* repo_resolve_commit(Repository* repo, const char* ref);
bool add_directory_recursive(Repository* repo, const char* dir_path);

bool repo_save(Repository* repo);
Repository* repo_load(const char* path);

int cmd_init(int argc, char* argv[]);
int cmd_add(int argc, char* argv[]);
int cmd_remove(int argc, char* argv[]);

#endif