#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "common.h"

char* str_dup(const char* str);
char* str_ndup(const char* str, size_t n);

char* path_join(const char* path1, const char* path2);
char* path_dirname(const char* path);
char* path_basename(const char* path);
bool path_is_absolute(const char* path);
char* path_normalize(const char* path);

char** str_split(const char* str, const char* delimiter, int* count);
void str_free_split(char** strings, int count);
char* str_trim(char* str);
bool str_starts_with(const char* str, const char* prefix);
bool str_ends_with(const char* str, const char* suffix);


char* str_format(const char* format, ...);
void str_tolower(char* str);
#endif