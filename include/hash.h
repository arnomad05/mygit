#ifndef HASH_H
#define HASH_H

#include "common.h"

typedef struct {
    uint8_t data[20];
} SHA1Hash;

SHA1Hash sha1_compute(const uint8_t* data, size_t size);
char* sha1_to_string(const SHA1Hash* hash);
SHA1Hash sha1_from_string(const char* str);
bool sha1_compare(const SHA1Hash* a, const SHA1Hash* b);

SHA1Hash hash_string(const char* str);
SHA1Hash hash_file(const char* filepath);
SHA1Hash hash_commit(const char* message, time_t timestamp,
    const SHA1Hash* parent, const char** files,
    const SHA1Hash* file_hashes, int file_count);

#endif