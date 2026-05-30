#define _CRT_SECURE_NO_WARNINGS
#include "file_ops.h"
#include "string_utils.h"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif

bool file_exists(const char* path) {
    if (!path) return false;
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(path, &st) == 0;
#endif
}

bool is_directory(const char* path) {
    if (!path) return false;
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
#endif
}

bool is_regular_file(const char* path) {
    if (!path) return false;
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISREG(st.st_mode);
#endif
}

bool create_directory(const char* path) {
    if (!path) return false;
#ifdef _WIN32
    return CreateDirectoryA(path, NULL) != 0;
#else
    return mkdir(path, 0755) == 0;
#endif
}

bool create_directories(const char* path) {
    if (!path) return false;

    char* normalized = str_dup(path);
    if (!normalized) return false;

    for (char* p = normalized; *p; p++) {
        if (*p == '\\') *p = '/';
    }

    char* parent = path_dirname(normalized);
    if (parent && strcmp(parent, ".") != 0 && strcmp(parent, "/") != 0) {
        if (!file_exists(parent)) {
            create_directories(parent);
        }
    }
    free(parent);

    if (!file_exists(normalized)) {
#ifdef _WIN32
        if (CreateDirectoryA(normalized, NULL) == 0) {
            free(normalized);
            return false;
        }
#else
        if (mkdir(normalized, 0755) != 0) {
            free(normalized);
            return false;
        }
#endif
    }

    free(normalized);
    return true;
}

bool delete_file(const char* path) {
    if (!path) return false;
#ifdef _WIN32
    return DeleteFileA(path) != 0;
#else
    return remove(path) == 0;
#endif
}

bool copy_file(const char* src, const char* dst) {
    if (!src || !dst) return false;

    FILE* src_file = fopen(src, "rb");
    if (!src_file) return false;

    FILE* dst_file = fopen(dst, "wb");
    if (!dst_file) {
        fclose(src_file);
        return false;
    }

    char buffer[8192];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dst_file) != bytes_read) {
            fclose(src_file);
            fclose(dst_file);
            return false;
        }
    }

    fclose(src_file);
    fclose(dst_file);
    return true;
}

bool move_file(const char* src, const char* dst) {
    if (!src || !dst) return false;
#ifdef _WIN32
    return MoveFileA(src, dst) != 0;
#else
    return rename(src, dst) == 0;
#endif
}

char* read_file_content(const char* path, size_t* size) {
    if (!path) return NULL;

    FILE* file = fopen(path, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size < 0) {
        fclose(file);
        return NULL;
    }

    char* content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(content, 1, file_size, file);
    content[bytes_read] = '\0';

    if (size) *size = bytes_read;

    fclose(file);
    return content;
}

bool write_file_content(const char* path, const char* content, size_t size) {
    if (!path || !content) return false;

    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s' for writing\n", path);
        return false;
    }

    size_t written = fwrite(content, 1, size, file);
    fclose(file);

    return written == size;
}

bool append_file_content(const char* path, const char* content, size_t size) {
    if (!path || !content) return false;

    FILE* file = fopen(path, "ab");
    if (!file) return false;

    size_t written = fwrite(content, 1, size, file);
    fclose(file);

    return written == size;
}

bool get_file_info(const char* path, FileInfo* info) {
    if (!path || !info) return false;

    memset(info, 0, sizeof(FileInfo));
    strncpy(info->full_path, path, MAX_PATH_LEN - 1);
    info->full_path[MAX_PATH_LEN - 1] = '\0';

    const char* basename = path_basename(path);
    if (basename) {
        strncpy(info->name, basename, MAX_FILENAME_LEN - 1);
        info->name[MAX_FILENAME_LEN - 1] = '\0';
        free((void*)basename);
    }

#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attrs)) {
        return false;
    }

    info->is_dir = (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    info->size = ((uint64_t)attrs.nFileSizeHigh << 32) | attrs.nFileSizeLow;

    ULARGE_INTEGER ull;
    ull.LowPart = attrs.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = attrs.ftLastWriteTime.dwHighDateTime;
    info->mtime = (time_t)((ull.QuadPart / 10000000ULL) - 11644473600ULL);
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;

    info->is_dir = S_ISDIR(st.st_mode);
    info->size = st.st_size;
    info->mtime = st.st_mtime;
#endif

    return true;
}

char* create_temp_file(const char* prefix) {
    const char* tmp_dir = NULL;

#ifdef _WIN32
    tmp_dir = getenv("TEMP");
    if (!tmp_dir) tmp_dir = getenv("TMP");
    if (!tmp_dir) tmp_dir = "C:\\Windows\\Temp";
#else
    tmp_dir = getenv("TMPDIR");
    if (!tmp_dir) tmp_dir = "/tmp";
#endif

    char* filename = (char*)malloc(MAX_PATH_LEN);
    if (!filename) return NULL;

    snprintf(filename, MAX_PATH_LEN, "%s/%s_%ld_%d.tmp",
        tmp_dir,
        prefix ? prefix : "tmp",
        (long)time(NULL),
        rand());

    if (file_exists(filename)) {
        snprintf(filename, MAX_PATH_LEN, "%s/%s_%ld_%d_%d.tmp",
            tmp_dir,
            prefix ? prefix : "tmp",
            (long)time(NULL),
            rand(),
            rand());
    }

    return filename;
}

char* create_temp_directory(const char* prefix) {
    const char* tmp_dir = NULL;

#ifdef _WIN32
    tmp_dir = getenv("TEMP");
    if (!tmp_dir) tmp_dir = getenv("TMP");
    if (!tmp_dir) tmp_dir = "C:\\Windows\\Temp";
#else
    tmp_dir = getenv("TMPDIR");
    if (!tmp_dir) tmp_dir = "/tmp";
#endif

    char* dirname = (char*)malloc(MAX_PATH_LEN);
    if (!dirname) return NULL;

    snprintf(dirname, MAX_PATH_LEN, "%s/%s_%ld_%d",
        tmp_dir,
        prefix ? prefix : "tmpdir",
        (long)time(NULL),
        rand());

    if (create_directory(dirname)) {
        return dirname;
    }

    free(dirname);
    return NULL;
}