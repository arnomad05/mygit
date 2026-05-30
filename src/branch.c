#define _CRT_SECURE_NO_WARNINGS
#include "branch.h"
#include "repo.h"
#include "file_ops.h"
#include "string_utils.h"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

BranchManager* branch_manager_create(void) {
    BranchManager* bm = (BranchManager*)calloc(1, sizeof(BranchManager));
    return bm;
}

void branch_manager_free(BranchManager* bm) {
    if (!bm) return;

    BranchNode* current = bm->branches;
    while (current) {
        BranchNode* next = current->next;
        free(current);
        current = next;
    }
    bm->branches = NULL;

    free(bm);
}

bool branch_create(BranchManager* bm, const char* name, const SHA1Hash* commit_hash) {
    if (!bm || !name || !commit_hash) return false;

    if (branch_find(bm, name)) {
        return false;
    }

    BranchNode* node = (BranchNode*)malloc(sizeof(BranchNode));
    if (!node) return false;

    strncpy(node->name, name, MAX_FILENAME_LEN - 1);
    node->name[MAX_FILENAME_LEN - 1] = '\0';
    node->commit_hash = *commit_hash;
    node->next = bm->branches;
    bm->branches = node;

    return true;
}

bool branch_delete(BranchManager* bm, const char* name) {
    if (!bm || !name) return false;

    if (strcmp(bm->current_branch, name) == 0) {
        return false;
    }

    BranchNode* prev = NULL;
    BranchNode* current = bm->branches;

    while (current) {
        if (strcmp(current->name, name) == 0) {
            if (prev) {
                prev->next = current->next;
            }
            else {
                bm->branches = current->next;
            }
            free(current);
            return true;
        }
        prev = current;
        current = current->next;
    }

    return false;
}

BranchNode* branch_find(BranchManager* bm, const char* name) {
    if (!bm || !name) return NULL;

    BranchNode* current = bm->branches;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

bool branch_checkout(BranchManager* bm, const char* name) {
    if (!bm || !name) return false;

    BranchNode* branch = branch_find(bm, name);
    if (!branch) return false;

    strncpy(bm->current_branch, name, MAX_FILENAME_LEN - 1);
    bm->current_branch[MAX_FILENAME_LEN - 1] = '\0';
    bm->is_detached = false;

    return true;
}

bool branch_save_to_file(BranchManager* bm, const char* repo_path) {
    if (!bm || !repo_path) return false;

    BranchNode* current = bm->branches;
    while (current) {
        char* head_path = path_join(repo_path, "refs/heads");
        head_path = path_join(head_path, current->name);

        if (head_path) {
            char* hash_str = sha1_to_string(&current->commit_hash);
            if (hash_str) {
                write_file_content(head_path, hash_str, strlen(hash_str));
                free(hash_str);
            }
            free(head_path);
        }

        current = current->next;
    }

    return true;
}

BranchManager* branch_load_from_file(const char* repo_path) {
    if (!repo_path) return NULL;

    BranchManager* bm = branch_manager_create();
    if (!bm) return NULL;

    char* heads_path = path_join(repo_path, "refs/heads");
    if (!heads_path) {
        branch_manager_free(bm);
        return NULL;
    }

    char* head_path = path_join(repo_path, "HEAD");
    if (head_path) {
        size_t head_size;
        char* head_content = read_file_content(head_path, &head_size);
        if (head_content) {
            char* newline = strchr(head_content, '\n');
            if (newline) *newline = '\0';

            if (strncmp(head_content, "ref: refs/heads/", 16) == 0) {
                strncpy(bm->current_branch, head_content + 16, MAX_FILENAME_LEN - 1);
                bm->current_branch[MAX_FILENAME_LEN - 1] = '\0';
                bm->is_detached = false;
            }
            else {
                bm->is_detached = true;
                bm->detached_hash = sha1_from_string(head_content);
            }

            free(head_content);
        }
        free(head_path);
    }

#ifdef _WIN32
    char search_path[MAX_PATH_LEN];
    snprintf(search_path, sizeof(search_path), "%s\\*", heads_path);

    WIN32_FIND_DATAA find_data;
    HANDLE find_handle = FindFirstFileA(search_path, &find_data);

    if (find_handle != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(find_data.cFileName, ".") == 0 ||
                strcmp(find_data.cFileName, "..") == 0) {
                continue;
            }

            char* branch_file = path_join(heads_path, find_data.cFileName);
            if (branch_file) {
                size_t hash_size;
                char* hash_content = read_file_content(branch_file, &hash_size);
                if (hash_content) {
                    if (hash_size > 0 && hash_content[hash_size - 1] == '\n') {
                        hash_content[hash_size - 1] = '\0';
                    }
                    SHA1Hash hash = sha1_from_string(hash_content);
                    branch_create(bm, find_data.cFileName, &hash);
                    free(hash_content);
                }
                free(branch_file);
            }
        } while (FindNextFileA(find_handle, &find_data));
        FindClose(find_handle);
    }
#else
    DIR* dir = opendir(heads_path);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char* branch_file = path_join(heads_path, entry->d_name);
            if (branch_file) {
                size_t hash_size;
                char* hash_content = read_file_content(branch_file, &hash_size);
                if (hash_content) {
                    if (hash_size > 0 && hash_content[hash_size - 1] == '\n') {
                        hash_content[hash_size - 1] = '\0';
                    }
                    SHA1Hash hash = sha1_from_string(hash_content);
                    branch_create(bm, entry->d_name, &hash);
                    free(hash_content);
                }
                free(branch_file);
            }
        }
        closedir(dir);
    }
#endif

    free(heads_path);
    return bm;
}

bool branch_checkout_commit(BranchManager* bm, const SHA1Hash* hash) {
    if (!bm || !hash) return false;

    bm->is_detached = true;
    bm->detached_hash = *hash;
    bm->current_branch[0] = '\0';

    return true;
}

char** branch_list_all(BranchManager* bm, int* count) {
    if (!bm || !count) return NULL;

    *count = 0;
    BranchNode* current = bm->branches;
    while (current) {
        (*count)++;
        current = current->next;
    }

    if (*count == 0) return NULL;

    char** list = (char**)malloc(sizeof(char*) * (*count));
    if (!list) {
        *count = 0;
        return NULL;
    }

    current = bm->branches;
    for (int i = 0; i < *count; i++) {
        list[i] = str_dup(current->name);
        current = current->next;
    }

    return list;
}

int cmd_branch(int argc, char* argv[]) {
    Repository* repo = repo_open(".");
    if (!repo) {
        fprintf(stderr, "%sError:%s Not a MyGit repository.\n",
            COLOR_RED, COLOR_RESET);
        return 1;
    }

    if (argc == 0) {
        BranchNode* current = repo->branch_manager->branches;
        while (current) {
            if (strcmp(current->name, repo->branch_manager->current_branch) == 0) {
                printf("%s* %s%s\n", COLOR_GREEN, current->name, COLOR_RESET);
            }
            else {
                printf("  %s\n", current->name);
            }
            current = current->next;
        }
    }
    else {
        if (branch_find(repo->branch_manager, argv[0])) {
            fprintf(stderr, "%sError:%s Branch '%s' already exists.\n",
                COLOR_RED, COLOR_RESET, argv[0]);
            repo_close(repo);
            return 1;
        }

        if (!branch_create(repo->branch_manager, argv[0],
            &repo->head_commit->hash)) {
            fprintf(stderr, "%sError:%s Cannot create branch.\n",
                COLOR_RED, COLOR_RESET);
            repo_close(repo);
            return 1;
        }

        printf("Created branch '%s'\n", argv[0]);
        repo_save(repo);
    }

    repo_close(repo);
    return 0;
}

int cmd_checkout(int argc, char* argv[]) {
    if (argc < 1) {
        fprintf(stderr, "Error: Specify branch, commit, or file.\n");
        fprintf(stderr, "Usage: mygit checkout <branch>\n");
        fprintf(stderr, "       mygit checkout <commit> <file>\n");
        return 1;
    }

    Repository* repo = repo_open(".");
    if (!repo) {
        fprintf(stderr, "Error: Not a MyGit repository.\n");
        return 1;
    }

    if (argc >= 2) {
        CommitNode* commit = repo_resolve_commit(repo, argv[0]);
        if (!commit) {
            SHA1Hash hash = sha1_from_string(argv[0]);
            commit = repo_find_commit(repo, &hash);
        }

        if (!commit) {
            fprintf(stderr, "Error: Commit '%s' not found.\n", argv[0]);
            repo_close(repo);
            return 1;
        }

        FileRecord* file = commit_find_file(commit, argv[1]);
        if (!file) {
            fprintf(stderr, "Error: File '%s' not found in commit %s.\n",
                argv[1], sha1_to_string(&commit->hash));
            repo_close(repo);
            return 1;
        }

        char* file_hash_str = sha1_to_string(&file->hash);
        char* subdir = str_ndup(file_hash_str, 2);
        char* filename = str_dup(file_hash_str + 2);

        char* object_path = path_join(repo->git_dir, "objects");
        char* object_subdir = path_join(object_path, subdir);
        char* object_file = path_join(object_subdir, filename);

        size_t size;
        char* content = read_file_content(object_file, &size);

        if (content) {
            char* dir = path_dirname(argv[1]);

            if (dir && strcmp(dir, ".") != 0 && !file_exists(dir)) {
                printf("Creating directory: %s\n", dir);
                if (!create_directories(dir)) {
                    fprintf(stderr, "Error: Cannot create directory '%s'\n", dir);
                    free(dir);
                    free(content);
                    repo_close(repo);
                    return 1;
                }
            }
            free(dir);

            if (write_file_content(argv[1], content, size)) {
                printf("Restored '%s' from commit %s\n",
                    argv[1], sha1_to_string(&commit->hash));
            }
            else {
                fprintf(stderr, "Error: Cannot write file '%s'\n", argv[1]);
            }

            free(content);
        }
        else {
            fprintf(stderr, "Error: Cannot read file content from objects.\n");
        }

        free(file_hash_str);
        free(subdir);
        free(filename);
        free(object_path);
        free(object_subdir);
        free(object_file);

        repo_close(repo);
        return 0;
    }

    BranchNode* branch = branch_find(repo->branch_manager, argv[0]);
    if (branch) {
        if (branch_checkout(repo->branch_manager, argv[0])) {
            CommitNode* commit = commit_load_from_hash(repo->git_dir,
                &branch->commit_hash);
            if (commit) {
                if (repo->head_commit) {
                    commit_free(repo->head_commit);
                }
                repo->head_commit = commit;
            }

            printf("Switched to branch '%s'\n", argv[0]);
            repo_save(repo);
        }
    }
    else {
        SHA1Hash hash = sha1_from_string(argv[0]);
        CommitNode* commit = repo_find_commit(repo, &hash);

        if (commit) {
            branch_checkout_commit(repo->branch_manager, &hash);

            if (repo->head_commit) {
                commit_free(repo->head_commit);
            }
            repo->head_commit = commit;

            char* hash_str = sha1_to_string(&hash);
            printf("Note: checking out '%s'.\n", hash_str);
            printf("You are in 'detached HEAD' state.\n");
            free(hash_str);
            repo_save(repo);
        }
        else {
            fprintf(stderr, "Error: Branch or commit '%s' not found.\n", argv[0]);
        }
    }

    repo_close(repo);
    return 0;
}