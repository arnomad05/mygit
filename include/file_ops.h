#ifndef FILE_OPS_H
#define FILE_OPS_H

#include "common.h"

bool file_exists(const char* path);
bool is_directory(const char* path);
bool is_regular_file(const char* path);
bool create_directory(const char* path);
bool create_directories(const char* path);
bool delete_file(const char* path);
bool copy_file(const char* src, const char* dst);
bool move_file(const char* src, const char* dst);

char* read_file_content(const char* path, size_t* size);
bool write_file_content(const char* path, const char* content, size_t size);
bool append_file_content(const char* path, const char* content, size_t size);

typedef struct {
    char name[MAX_FILENAME_LEN];
    char full_path[MAX_PATH_LEN];
    bool is_dir;
    uint64_t size;
    time_t mtime;
} FileInfo;

typedef bool (*FileVisitor)(const FileInfo* file, void* user_data);
bool iterate_directory(const char* path, FileVisitor visitor, void* user_data, bool recursive);
bool get_file_info(const char* path, FileInfo* info);

char* create_temp_file(const char* prefix);
char* create_temp_directory(const char* prefix);

#endif