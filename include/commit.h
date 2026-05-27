#ifndef COMMIT_H
#define COMMIT_H

#include "common.h"
#include "hash.h"

typedef struct {
	char filename[MAX_FILENAME_LEN];
	SHA1Hash hash;
	enum { FILE_ADDED, FILE_MODIFIED, FILE_DELETED } action;
	char* content;
} FileRecord;

typedef struct CommitNode {
    SHA1Hash hash;
    char message[MAX_MESSAGE_LEN];
    time_t timestamp;
    SHA1Hash parent_hash;
    bool has_parent;

    FileRecord* files;
    int file_count;
    int files_capacity;

    struct CommitNode* next;
    struct CommitNode* prev;
} CommitNode;

CommitNode* commit_create(const char* message, const SHA1Hash* parent_hash,
    FileRecord* files, int file_count);
void commit_free(CommitNode* commit);
bool commit_add_file(CommitNode* commit, const char* filename,
    const SHA1Hash* hash, int action);
FileRecord* commit_find_file(CommitNode* commit, const char* filename);
bool commit_save_to_file(CommitNode* commit, const char* repo_path);
CommitNode* commit_load_from_hash(const char* repo_path, const SHA1Hash* hash);
char* commit_to_string(CommitNode* commit);
CommitNode* commit_from_string(const char* str);

int cmd_commit(int argc, char* argv[]);

#endif