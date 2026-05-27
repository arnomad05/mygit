#define _CRT_SECURE_NO_WARNINGS
#include "commit.h"
#include "repo.h"
#include "file_ops.h"
#include "string_utils.h"

CommitNode* commit_create(const char* message, const SHA1Hash* parent_hash,
    FileRecord* files, int file_count) {
    CommitNode* commit = (CommitNode*)calloc(1, sizeof(CommitNode));
    if (!commit) return NULL;

    if (message) {
        strncpy(commit->message, message, MAX_MESSAGE_LEN - 1);
        commit->message[MAX_MESSAGE_LEN - 1] = '\0';
    }
    else {
        commit->message[0] = '\0';
    }

    commit->timestamp = time(NULL);

    if (parent_hash) {
        commit->parent_hash = *parent_hash;
        commit->has_parent = true;
    }
    else {
        commit->has_parent = false;
    }

    if (files && file_count > 0) {
        commit->files = (FileRecord*)malloc(sizeof(FileRecord) * file_count);
        if (commit->files) {
            memcpy(commit->files, files, sizeof(FileRecord) * file_count);

            for (int i = 0; i < file_count; i++) {
                if (files[i].content) {
                    commit->files[i].content = str_dup(files[i].content);
                }
                else {
                    commit->files[i].content = NULL;
                }
            }

            commit->file_count = file_count;
            commit->files_capacity = file_count;
        }
    }

    char** filenames = NULL;
    SHA1Hash* file_hashes = NULL;

    if (file_count > 0) {
        filenames = (char**)malloc(sizeof(char*) * file_count);
        file_hashes = (SHA1Hash*)malloc(sizeof(SHA1Hash) * file_count);

        if (filenames && file_hashes) {
            for (int i = 0; i < file_count; i++) {
                filenames[i] = files[i].filename;
                file_hashes[i] = files[i].hash;
            }
        }
    }

    commit->hash = hash_commit(message, commit->timestamp,
        parent_hash,
        (const char**)filenames,
        file_hashes, file_count);

    free(filenames);
    free(file_hashes);

    return commit;
}

void commit_free(CommitNode* commit) {
    if (!commit) return;

    if (commit->files) {
        for (int i = 0; i < commit->file_count; i++) {
            free(commit->files[i].content);
            commit->files[i].content = NULL;
        }
        free(commit->files);
        commit->files = NULL;
    }

    if (commit->prev) {
        commit->prev->next = commit->next;
    }
    if (commit->next) {
        commit->next->prev = commit->prev;
    }

    free(commit);
}

bool commit_add_file(CommitNode* commit, const char* filename,
    const SHA1Hash* hash, int action) {
    if (!commit || !filename || !hash) return false;

    for (int i = 0; i < commit->file_count; i++) {
        if (strcmp(commit->files[i].filename, filename) == 0) {
            commit->files[i].hash = *hash;
            commit->files[i].action = action;
            return true;
        }
    }

    if (commit->file_count >= commit->files_capacity) {
        int new_capacity = commit->files_capacity == 0 ? 10 :
            commit->files_capacity * 2;
        FileRecord* new_files = (FileRecord*)realloc(commit->files,
            sizeof(FileRecord) * new_capacity);
        if (!new_files) return false;

        commit->files = new_files;
        commit->files_capacity = new_capacity;
    }

    FileRecord* record = &commit->files[commit->file_count];
    strncpy(record->filename, filename, MAX_FILENAME_LEN - 1);
    record->filename[MAX_FILENAME_LEN - 1] = '\0';
    record->hash = *hash;
    record->action = action;
    record->content = NULL;

    commit->file_count++;
    return true;
}

FileRecord* commit_find_file(CommitNode* commit, const char* filename) {
    if (!commit || !filename) return NULL;

    for (int i = 0; i < commit->file_count; i++) {
        if (strcmp(commit->files[i].filename, filename) == 0) {
            return &commit->files[i];
        }
    }

    return NULL;
}

bool commit_save_to_file(CommitNode* commit, const char* repo_path) {
    if (!commit || !repo_path) return false;

    char* hash_str = sha1_to_string(&commit->hash);
    if (!hash_str) return false;

    char* subdir = str_ndup(hash_str, 2);
    char* filename = str_dup(hash_str + 2);

    char* subdir_path = path_join(repo_path, "objects");
    subdir_path = path_join(subdir_path, subdir);
    create_directories(subdir_path);

    char* file_path = path_join(subdir_path, filename);

    free(hash_str);
    free(subdir);
    free(filename);
    free(subdir_path);

    char* content = commit_to_string(commit);
    if (!content) {
        free(file_path);
        return false;
    }

    bool success = write_file_content(file_path, content, strlen(content));

    free(content);
    free(file_path);

    for (int i = 0; i < commit->file_count; i++) {
        if (commit->files[i].content) {
            char* file_hash_str = sha1_to_string(&commit->files[i].hash);
            char* file_subdir = str_ndup(file_hash_str, 2);
            char* file_name = str_dup(file_hash_str + 2);

            char* file_subdir_path = path_join(repo_path, "objects");
            file_subdir_path = path_join(file_subdir_path, file_subdir);
            create_directories(file_subdir_path);

            char* file_path2 = path_join(file_subdir_path, file_name);

            write_file_content(file_path2, commit->files[i].content,
                strlen(commit->files[i].content));

            free(file_hash_str);
            free(file_subdir);
            free(file_name);
            free(file_subdir_path);
            free(file_path2);
        }
    }

    return success;
}

char* commit_to_string(CommitNode* commit) {
    if (!commit) return NULL;

    size_t size = 1024;
    for (int i = 0; i < commit->file_count; i++) {
        size += 256;
    }

    char* str = (char*)malloc(size);
    if (!str) return NULL;

    int offset = 0;
    offset += snprintf(str + offset, size - offset,
        "commit %ld\n", (long)commit->timestamp);
    offset += snprintf(str + offset, size - offset,
        "message %s\n", commit->message);

    if (commit->has_parent) {
        char* parent_str = sha1_to_string(&commit->parent_hash);
        offset += snprintf(str + offset, size - offset,
            "parent %s\n", parent_str);
        free(parent_str);
    }

    for (int i = 0; i < commit->file_count; i++) {
        char* file_hash_str = sha1_to_string(&commit->files[i].hash);
        offset += snprintf(str + offset, size - offset,
            "file %s %s %d\n",
            file_hash_str,
            commit->files[i].filename,
            commit->files[i].action);
        free(file_hash_str);
    }

    return str;
}

CommitNode* commit_from_string(const char* str) {
    if (!str) return NULL;

    CommitNode* commit = (CommitNode*)calloc(1, sizeof(CommitNode));
    if (!commit) return NULL;

    commit->files_capacity = 10;
    commit->files = (FileRecord*)malloc(sizeof(FileRecord) * commit->files_capacity);
    if (!commit->files) {
        free(commit);
        return NULL;
    }

    int line_count;
    char** lines = str_split(str, "\n", &line_count);
    if (!lines) {
        free(commit->files);
        free(commit);
        return NULL;
    }

    for (int i = 0; i < line_count; i++) {
        char* line = str_trim(lines[i]);
        if (strlen(line) == 0) continue;

        if (str_starts_with(line, "commit ")) {
            commit->timestamp = atol(line + 7);
        }
        else if (str_starts_with(line, "message ")) {
            strncpy(commit->message, line + 8, MAX_MESSAGE_LEN - 1);
            commit->message[MAX_MESSAGE_LEN - 1] = '\0';
        }
        else if (str_starts_with(line, "parent ")) {
            commit->parent_hash = sha1_from_string(line + 7);
            commit->has_parent = true;
        }
        else if (str_starts_with(line, "file ")) {
            int parts_count;
            char** parts = str_split(line + 5, " ", &parts_count);

            if (parts && parts_count >= 3) {
                if (commit->file_count < commit->files_capacity) {
                    FileRecord* record = &commit->files[commit->file_count];
                    record->hash = sha1_from_string(parts[0]);
                    strncpy(record->filename, parts[1], MAX_FILENAME_LEN - 1);
                    record->filename[MAX_FILENAME_LEN - 1] = '\0';
                    record->action = atoi(parts[2]);
                    record->content = NULL;
                    commit->file_count++;
                }
            }

            if (parts) {
                str_free_split(parts, parts_count);
            }
        }
    }

    str_free_split(lines, line_count);

    char** filenames = NULL;
    SHA1Hash* file_hashes = NULL;

    if (commit->file_count > 0) {
        filenames = (char**)malloc(sizeof(char*) * commit->file_count);
        file_hashes = (SHA1Hash*)malloc(sizeof(SHA1Hash) * commit->file_count);

        if (filenames && file_hashes) {
            for (int i = 0; i < commit->file_count; i++) {
                filenames[i] = commit->files[i].filename;
                file_hashes[i] = commit->files[i].hash;
            }
        }
    }

    commit->hash = hash_commit(commit->message, commit->timestamp,
        commit->has_parent ? &commit->parent_hash : NULL,
        (const char**)filenames,
        file_hashes,
        commit->file_count);

    free(filenames);
    free(file_hashes);

    return commit;
}

CommitNode* commit_load_from_hash(const char* repo_path, const SHA1Hash* hash) {
    if (!repo_path || !hash) return NULL;

    char* hash_str = sha1_to_string(hash);
    if (!hash_str) return NULL;

    char* subdir = str_ndup(hash_str, 2);
    char* filename = str_dup(hash_str + 2);

    char* file_path = path_join(repo_path, "objects");
    file_path = path_join(file_path, subdir);
    file_path = path_join(file_path, filename);

    free(hash_str);
    free(subdir);
    free(filename);

    if (!file_exists(file_path)) {
        free(file_path);
        return NULL;
    }

    size_t size;
    char* content = read_file_content(file_path, &size);
    free(file_path);

    if (!content) return NULL;

    CommitNode* commit = commit_from_string(content);
    free(content);

    return commit;
}

int cmd_commit(int argc, char* argv[]) {
    if (argc < 1) {
        fprintf(stderr, "Error: Commit message required.\n");
        fprintf(stderr, "Usage: mygit commit <message>\n");
        return 1;
    }

    Repository* repo = repo_open(".");
    if (!repo) {
        fprintf(stderr, "Error: Not a MyGit repository.\n");
        return 1;
    }

    if (repo->staged_count == 0) {
        fprintf(stderr, "Error: Nothing to commit.\n");
        fprintf(stderr, "Use 'mygit add' to stage files.\n");
        repo_close(repo);
        return 1;
    }

    FileRecord* files_copy = (FileRecord*)malloc(sizeof(FileRecord) * repo->staged_count);
    if (!files_copy) {
        fprintf(stderr, "Error: Out of memory.\n");
        repo_close(repo);
        return 1;
    }

    memcpy(files_copy, repo->staged_files, sizeof(FileRecord) * repo->staged_count);

    for (int i = 0; i < repo->staged_count; i++) {
        if (repo->staged_files[i].action != FILE_DELETED) {
            files_copy[i].content = read_file_content(
                repo->staged_files[i].filename, NULL);
        }
        else {
            files_copy[i].content = NULL;
        }
    }

    CommitNode* new_commit = commit_create(
        argv[0],
        repo->head_commit ? &repo->head_commit->hash : NULL,
        files_copy,
        repo->staged_count
    );

    for (int i = 0; i < repo->staged_count; i++) {
        free(files_copy[i].content);
    }
    free(files_copy);

    if (!new_commit) {
        fprintf(stderr, "Error: Cannot create commit.\n");
        repo_close(repo);
        return 1;
    }

    if (!commit_save_to_file(new_commit, repo->git_dir)) {
        fprintf(stderr, "Error: Cannot save commit.\n");
        commit_free(new_commit);
        repo_close(repo);
        return 1;
    }

    if (repo->head_commit) {
        new_commit->prev = repo->head_commit;
        repo->head_commit->next = new_commit;
    }
    repo->head_commit = new_commit;

    BranchNode* branch = branch_find(repo->branch_manager,
        repo->branch_manager->current_branch);
    if (branch) {
        branch->commit_hash = new_commit->hash;
    }

    repo_clear_staging(repo);
    repo_save(repo);

    char* hash_str = sha1_to_string(&new_commit->hash);
    printf("[%s] %s\n", hash_str, new_commit->message);
    free(hash_str);

    repo_close(repo);
    return 0;
}