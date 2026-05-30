#ifndef BRANCH_H
#define BRANCH_H

#include "common.h"
#include "hash.h"

typedef struct CommitNode CommitNode;

typedef struct BranchNode {
    char name[MAX_FILENAME_LEN];
    SHA1Hash commit_hash;
    struct BranchNode* next;
} BranchNode;

typedef struct {
    BranchNode* branches;
    char current_branch[MAX_FILENAME_LEN];
    bool is_detached;
    SHA1Hash detached_hash;
} BranchManager;

BranchManager* branch_manager_create(void);
void branch_manager_free(BranchManager* bm);
bool branch_create(BranchManager* bm, const char* name, const SHA1Hash* commit_hash);
bool branch_delete(BranchManager* bm, const char* name);
BranchNode* branch_find(BranchManager* bm, const char* name);
bool branch_checkout(BranchManager* bm, const char* name);
bool branch_checkout_commit(BranchManager* bm, const SHA1Hash* hash);
bool branch_save_to_file(BranchManager* bm, const char* repo_path);
BranchManager* branch_load_from_file(const char* repo_path);
char** branch_list_all(BranchManager* bm, int* count);

int cmd_branch(int argc, char* argv[]);
int cmd_checkout(int argc, char* argv[]);

#endif