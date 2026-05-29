#define _CRT_SECURE_NO_WARNINGS
#include "repo.h"
#include "file_ops.h"
#include "string_utils.h"
#include "hash.h"

Repository* repo_init(const char* path) {
    if (!path) return NULL;

    char* git_path = path_join(path, ".mygit");
    if (!git_path) return NULL;

    if (file_exists(git_path)) {
        fprintf(stderr, "%sError:%s Repository already exists.\n",
            COLOR_RED, COLOR_RESET);
        free(git_path);
        return NULL;
    }

    Repository* repo = (Repository*)calloc(1, sizeof(Repository));
    if (!repo) {
        free(git_path);
        return NULL;
    }

    strncpy(repo->repo_path, path, MAX_PATH_LEN - 1);
    strncpy(repo->git_dir, git_path, MAX_PATH_LEN - 1);
    free(git_path);

    if (!create_directories(repo->git_dir)) {
        fprintf(stderr, "%sError:%s Cannot create .mygit directory\n",
            COLOR_RED, COLOR_RESET);
        free(repo);
        return NULL;
    }

    char* objects_dir = path_join(repo->git_dir, "objects");
    char* refs_dir = path_join(repo->git_dir, "refs");
    char* heads_dir = path_join(repo->git_dir, "refs/heads");

    if (!objects_dir || !refs_dir || !heads_dir) {
        fprintf(stderr, "%sError:%s Memory allocation failed\n",
            COLOR_RED, COLOR_RESET);
        free(objects_dir);
        free(refs_dir);
        free(heads_dir);
        free(repo);
        return NULL;
    }

    create_directories(objects_dir);
    create_directories(refs_dir);
    create_directories(heads_dir);

    free(objects_dir);
    free(refs_dir);
    free(heads_dir);

    CommitNode* initial = commit_create("Initial commit", NULL, NULL, 0);
    if (!initial) {
        free(repo);
        return NULL;
    }

    repo->head_commit = initial;

    repo->branch_manager = branch_manager_create();
    if (!repo->branch_manager) {
        commit_free(initial);
        free(repo);
        return NULL;
    }

    branch_create(repo->branch_manager, "master", &initial->hash);
    strncpy(repo->branch_manager->current_branch, "master", MAX_FILENAME_LEN - 1);
    repo->branch_manager->is_detached = false;

    repo->staged_files = NULL;
    repo->staged_count = 0;
    repo->staged_capacity = 0;

    repo->tracked_files = NULL;
    repo->tracked_count = 0;
    repo->tracked_capacity = 0;

    repo->is_initialized = true;

    if (!repo_save(repo)) {
        fprintf(stderr, "%sError:%s Cannot save repository\n",
            COLOR_RED, COLOR_RESET);
        repo_close(repo);
        return NULL;
    }

    printf("%sInitialized empty MyGit repository in %s%s\n",
        COLOR_GREEN, repo->git_dir, COLOR_RESET);

    return repo;
}

Repository* repo_open(const char* path) {
    if (!path) return NULL;

    char* repo_path = NULL;

    char* current = str_dup(path);
    if (!current) return NULL;

    while (current) {
        char* git_path = path_join(current, ".mygit");
        if (git_path && file_exists(git_path)) {
            repo_path = str_dup(current);
            free(git_path);
            free(current);
            break;
        }
        free(git_path);

        char* parent = path_dirname(current);
        if (strcmp(parent, current) == 0) {
            free(parent);
            free(current);
            break;
        }
        free(current);
        current = parent;
    }

    if (!repo_path) return NULL;

    Repository* repo = repo_load(repo_path);
    free(repo_path);

    return repo;
}

void repo_close(Repository* repo) {
    if (!repo) return;

    repo_clear_staging(repo);

    if (repo->head_commit) {
        commit_free(repo->head_commit);
        repo->head_commit = NULL;
    }

    if (repo->branch_manager) {
        branch_manager_free(repo->branch_manager);
        repo->branch_manager = NULL;
    }

    if (repo->tracked_files) {
        for (int i = 0; i < repo->tracked_count; i++) {
            free(repo->tracked_files[i]);
        }
        free(repo->tracked_files);
        repo->tracked_files = NULL;
    }

    free(repo);
}

bool repo_is_valid(const char* path) {
    if (!path) return false;
    char* git_path = path_join(path, ".mygit");
    if (!git_path) return false;

    bool valid = file_exists(git_path) && is_directory(git_path);
    free(git_path);
    return valid;
}

bool repo_stage_file(Repository* repo, const char* filename) {
    if (!repo || !filename) return false;

    for (int i = 0; i < repo->staged_count; i++) {
        if (strcmp(repo->staged_files[i].filename, filename) == 0) {
            return true;
        }
    }

    if (repo->staged_count >= repo->staged_capacity) {
        int new_capacity = repo->staged_capacity == 0 ? 10 : repo->staged_capacity * 2;
        FileRecord* new_files = (FileRecord*)realloc(repo->staged_files,
            sizeof(FileRecord) * new_capacity);
        if (!new_files) return false;

        repo->staged_files = new_files;
        repo->staged_capacity = new_capacity;
    }

    SHA1Hash file_hash = hash_file(filename);

    FileRecord* record = &repo->staged_files[repo->staged_count];
    strncpy(record->filename, filename, MAX_FILENAME_LEN - 1);
    record->hash = file_hash;
    record->action = FILE_ADDED;
    record->content = NULL;

    repo->staged_count++;

    return true;
}

bool repo_unstage_file(Repository* repo, const char* filename) {
    if (!repo || !filename) return false;

    for (int i = 0; i < repo->staged_count; i++) {
        if (strcmp(repo->staged_files[i].filename, filename) == 0) {
            free(repo->staged_files[i].content);
            for (int j = i; j < repo->staged_count - 1; j++) {
                repo->staged_files[j] = repo->staged_files[j + 1];
            }
            repo->staged_count--;
            return true;
        }
    }

    return false;
}

bool repo_stage_remove(Repository* repo, const char* filename) {
    if (!repo || !filename) return false;

    if (repo->staged_count >= repo->staged_capacity) {
        int new_capacity = repo->staged_capacity == 0 ? 10 : repo->staged_capacity * 2;
        FileRecord* new_files = (FileRecord*)realloc(repo->staged_files,
            sizeof(FileRecord) * new_capacity);
        if (!new_files) return false;

        repo->staged_files = new_files;
        repo->staged_capacity = new_capacity;
    }

    FileRecord* record = &repo->staged_files[repo->staged_count];
    strncpy(record->filename, filename, MAX_FILENAME_LEN - 1);
    memset(&record->hash, 0, sizeof(SHA1Hash));
    record->action = FILE_DELETED;
    record->content = NULL;

    repo->staged_count++;

    printf("Staged removal of '%s'\n", filename);
    return true;
}

void repo_clear_staging(Repository* repo) {
    if (!repo) return;

    if (repo->staged_files) {
        for (int i = 0; i < repo->staged_count; i++) {
            if (repo->staged_files[i].content) {
                free(repo->staged_files[i].content);
                repo->staged_files[i].content = NULL;
            }
        }
        free(repo->staged_files);
        repo->staged_files = NULL;
    }

    repo->staged_count = 0;
    repo->staged_capacity = 0;
}

CommitNode* repo_get_head_commit(Repository* repo) {
    if (!repo) return NULL;
    return repo->head_commit;
}

bool repo_add_commit(Repository* repo, CommitNode* commit) {
    if (!repo || !commit) return false;

    if (repo->head_commit) {
        commit->prev = repo->head_commit;
        repo->head_commit->next = commit;
    }

    repo->head_commit = commit;
    return true;
}

CommitNode* repo_find_commit(Repository* repo, const SHA1Hash* hash) {
    if (!repo || !hash) return NULL;

    CommitNode* current = repo->head_commit;
    while (current) {
        if (sha1_compare(&current->hash, hash)) {
            return current;
        }
        current = current->prev;
    }

    return NULL;
}

CommitNode* repo_resolve_commit(Repository* repo, const char* ref) {
    if (!repo || !ref) return NULL;

    if (strlen(ref) == 40) {
        SHA1Hash hash = sha1_from_string(ref);
        CommitNode* commit = repo_find_commit(repo, &hash);
        if (commit) {
            branch_checkout_commit(repo->branch_manager, &hash);
        }
        return commit;
    }

    BranchNode* branch = branch_find(repo->branch_manager, ref);
    if (branch) {
        return repo_find_commit(repo, &branch->commit_hash);
    }

    return NULL;
}

bool repo_save(Repository* repo) {
    if (!repo) return false;

    CommitNode* current = repo->head_commit;
    while (current) {
        commit_save_to_file(current, repo->git_dir);
        current = current->prev;
    }

    char* head_path = path_join(repo->git_dir, "HEAD");
    if (!head_path) return false;

    char* head_content;
    if (repo->branch_manager->is_detached) {
        char* hash_str = sha1_to_string(&repo->branch_manager->detached_hash);
        head_content = str_format("%s\n", hash_str);
        free(hash_str);
    }
    else {
        head_content = str_format("ref: refs/heads/%s\n",
            repo->branch_manager->current_branch);
    }

    if (head_content) {
        write_file_content(head_path, head_content, strlen(head_content));
        free(head_content);
    }
    free(head_path);

    if (repo->head_commit) {
        commit_save_to_file(repo->head_commit, repo->git_dir);
    }

    branch_save_to_file(repo->branch_manager, repo->git_dir);

    char* index_path = path_join(repo->git_dir, "index");
    if (!index_path) return false;

    FILE* index_file = fopen(index_path, "wb");
    if (index_file) {
        fprintf(index_file, "%d\n", repo->staged_count);
        for (int i = 0; i < repo->staged_count; i++) {
            char* hash_str = sha1_to_string(&repo->staged_files[i].hash);
            fprintf(index_file, "%s %s %d\n",
                hash_str,
                repo->staged_files[i].filename,
                repo->staged_files[i].action);
            free(hash_str);
        }
        fclose(index_file);
    }
    free(index_path);

    return true;
}

Repository* repo_load(const char* path) {
    if (!path) return NULL;

    char* git_path = path_join(path, ".mygit");
    if (!git_path) return NULL;

    if (!file_exists(git_path)) {
        free(git_path);
        return NULL;
    }

    Repository* repo = (Repository*)calloc(1, sizeof(Repository));
    if (!repo) {
        free(git_path);
        return NULL;
    }

    strncpy(repo->repo_path, path, MAX_PATH_LEN - 1);
    strncpy(repo->git_dir, git_path, MAX_PATH_LEN - 1);
    free(git_path);

    char* head_path = path_join(repo->git_dir, "HEAD");
    if (head_path) {
        size_t head_size;
        char* head_content = read_file_content(head_path, &head_size);
        if (head_content) {
            if (head_size > 0 && head_content[head_size - 1] == '\n') {
                head_content[head_size - 1] = '\0';
                head_size--;
            }

            if (strncmp(head_content, "ref: refs/heads/", 16) == 0) {
                if (repo->branch_manager) {
                    strncpy(repo->branch_manager->current_branch,
                        head_content + 16, MAX_FILENAME_LEN - 1);
                }
            }
            free(head_content);
        }
        free(head_path);
    }

    BranchManager* bm = branch_load_from_file(repo->git_dir);
    if (bm) {
        repo->branch_manager = bm;
    }
    else {
        repo->branch_manager = branch_manager_create();
    }

    if (!repo->branch_manager->is_detached &&
        strlen(repo->branch_manager->current_branch) > 0) {
        BranchNode* branch = branch_find(repo->branch_manager,
            repo->branch_manager->current_branch);
        if (branch) {
            repo->head_commit = commit_load_from_hash(repo->git_dir,
                &branch->commit_hash);

            if (repo->head_commit) {
                CommitNode* current = repo->head_commit;
                while (current->has_parent) {
                    CommitNode* parent = commit_load_from_hash(
                        repo->git_dir, &current->parent_hash);
                    if (parent) {
                        current->prev = parent;
                        parent->next = current;
                        current = parent;
                    }
                    else {
                        break;
                    }
                }
            }
        }
    }

    if (!repo->head_commit) {
        commit_free(repo->head_commit);
        repo->head_commit = NULL;
    }

    char* index_path = path_join(repo->git_dir, "index");
    if (index_path && file_exists(index_path)) {
        FILE* index_file = fopen(index_path, "rb");
        if (index_file) {
            int count;
            if (fscanf(index_file, "%d\n", &count) == 1) {
                repo->staged_capacity = count + 10;
                repo->staged_files = (FileRecord*)calloc(repo->staged_capacity,
                    sizeof(FileRecord));

                for (int i = 0; i < count; i++) {
                    char hash_str[MAX_HASH_LEN];
                    char filename[MAX_FILENAME_LEN];
                    int action;

                    if (fscanf(index_file, "%40s %255s %d\n",
                        hash_str, filename, &action) == 3) {
                        FileRecord* record = &repo->staged_files[repo->staged_count];
                        strncpy(record->filename, filename, MAX_FILENAME_LEN - 1);
                        record->hash = sha1_from_string(hash_str);
                        record->action = action;
                        record->content = NULL;
                        repo->staged_count++;
                    }
                }
            }
            fclose(index_file);
        }
        free(index_path);
    }

    repo->is_initialized = true;
    return repo;
}

int cmd_init(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    char cwd[MAX_PATH_LEN];
    if (!getcwd(cwd, sizeof(cwd))) {
        fprintf(stderr, "%sError:%s Cannot get current directory\n",
            COLOR_RED, COLOR_RESET);
        return 1;
    }

    Repository* repo = repo_init(cwd);
    if (!repo) {
        return 1;
    }

    repo_close(repo);
    return 0;
}

int cmd_add(int argc, char* argv[]) {
    if (argc < 1) {
        fprintf(stderr, "Error: No files specified.\n");
        fprintf(stderr, "Usage: mygit add <file>\n");
        return 1;
    }

    Repository* repo = repo_open(".");
    if (!repo) {
        fprintf(stderr, "Error: Not a MyGit repository.\n");
        return 1;
    }
    
    bool success = true;

    for (int i = 0; i < argc; i++) {
        if (!file_exists(argv[i])) {
            fprintf(stderr, "Error: '%s' not found.\n", argv[i]);
            success = false;
            continue;
        }

        if (!repo_stage_file(repo, argv[i])) {
            fprintf(stderr, "Error: Cannot stage '%s'\n", argv[i]);
            success = false;
        }
        else {
            printf("Staged '%s'\n", argv[i]);
        }
    }

    repo_save(repo);
    repo_close(repo);
    return success ? 0 : 1;
}

int cmd_remove(int argc, char* argv[]) {
    if (argc < 1) {
        fprintf(stderr, "%sError:%s No files specified.\n",
            COLOR_RED, COLOR_RESET);
        fprintf(stderr, "Usage: mygit remove <file>\n");
        return 1;
    }

    Repository* repo = repo_open(".");
    if (!repo) {
        fprintf(stderr, "%sError:%s Not a MyGit repository.\n",
            COLOR_RED, COLOR_RESET);
        return 1;
    }

    for (int i = 0; i < argc; i++) {
        if (!repo_stage_remove(repo, argv[i])) {
            fprintf(stderr, "%sError:%s Cannot stage removal of '%s'\n",
                COLOR_RED, COLOR_RESET, argv[i]);
        }
    }

    repo_save(repo);
    repo_close(repo);
    return 0;
}
