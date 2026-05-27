#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

#define MAX_PATH_LEN 4096
#define MAX_HASH_LEN 41
#define MAX_LINE_LEN 1024
#define MAX_FILENAME_LEN 256
#define MAX_MESSAGE_LEN 1024

#ifdef _WIN32
#define COLOR_RED     ""
#define COLOR_GREEN   ""
#define COLOR_YELLOW  ""
#define COLOR_BLUE    ""
#define COLOR_RESET   ""
#else
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RESET   "\033[0m"
#endif

#endif