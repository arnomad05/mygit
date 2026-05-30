#define _CRT_SECURE_NO_WARNINGS
#include "hash.h"
#include "file_ops.h"
#include "string_utils.h"
#include <ctype.h>

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t buffer[64];
} SHA1_CTX;

#define SHA1_ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static void sha1_transform(uint32_t state[5], const uint8_t buffer[64]) {
    uint32_t a, b, c, d, e, w[80];

    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)buffer[i * 4] << 24) |
            ((uint32_t)buffer[i * 4 + 1] << 16) |
            ((uint32_t)buffer[i * 4 + 2] << 8) |
            (uint32_t)buffer[i * 4 + 3];
    }

    for (int i = 16; i < 80; i++) {
        w[i] = SHA1_ROTL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    for (int i = 0; i < 80; i++) {
        uint32_t f, k;

        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        }
        else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        }
        else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        }
        else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        uint32_t temp = SHA1_ROTL(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = SHA1_ROTL(b, 30);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

static void sha1_init(SHA1_CTX* ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count[0] = 0;
    ctx->count[1] = 0;
}

static void sha1_update(SHA1_CTX* ctx, const uint8_t* data, size_t len) {
    size_t i, j;

    j = (ctx->count[0] >> 3) & 63;
    if ((ctx->count[0] += len << 3) < (len << 3)) {
        ctx->count[1]++;
    }
    ctx->count[1] += len >> 29;

    if ((j + len) > 63) {
        memcpy(&ctx->buffer[j], data, (i = 64 - j));
        sha1_transform(ctx->state, ctx->buffer);
        for (; i + 63 < len; i += 64) {
            sha1_transform(ctx->state, &data[i]);
        }
        j = 0;
    }
    else {
        i = 0;
    }

    memcpy(&ctx->buffer[j], &data[i], len - i);
}

static void sha1_final(SHA1_CTX* ctx, uint8_t digest[20]) {
    uint8_t finalcount[8];

    for (int i = 0; i < 8; i++) {
        finalcount[i] = (uint8_t)((ctx->count[(i >= 4 ? 0 : 1)] >>
            ((3 - (i & 3)) * 8)) & 255);
    }

    sha1_update(ctx, (const uint8_t*)"\200", 1);

    while ((ctx->count[0] & 504) != 448) {
        sha1_update(ctx, (const uint8_t*)"\0", 1);
    }

    sha1_update(ctx, finalcount, 8);

    for (int i = 0; i < 20; i++) {
        digest[i] = (uint8_t)((ctx->state[i >> 2] >>
            ((3 - (i & 3)) * 8)) & 255);
    }
}

SHA1Hash sha1_compute(const uint8_t* data, size_t size) {
    SHA1_CTX ctx;
    SHA1Hash hash;

    sha1_init(&ctx);
    sha1_update(&ctx, data, size);
    sha1_final(&ctx, hash.data);

    return hash;
}

char* sha1_to_string(const SHA1Hash* hash) {
    if (!hash) return NULL;

    char* str = (char*)malloc(41);
    if (!str) return NULL;

    for (int i = 0; i < 20; i++) {
        sprintf(str + i * 2, "%02x", hash->data[i]);
    }
    str[40] = '\0';

    return str;
}

SHA1Hash sha1_from_string(const char* str) {
    SHA1Hash hash;
    memset(&hash, 0, sizeof(hash));

    if (!str || strlen(str) != 40) return hash;

    for (int i = 0; i < 20; i++) {
        char hex[3] = { str[i * 2], str[i * 2 + 1], '\0' };
        hash.data[i] = (uint8_t)strtoul(hex, NULL, 16);
    }

    return hash;
}

bool sha1_compare(const SHA1Hash* a, const SHA1Hash* b) {
    if (!a || !b) return false;
    return memcmp(a->data, b->data, 20) == 0;
}

SHA1Hash hash_string(const char* str) {
    return sha1_compute((const uint8_t*)str, strlen(str));
}

SHA1Hash hash_file(const char* filepath) {
    SHA1Hash empty_hash;
    memset(&empty_hash, 0, sizeof(empty_hash));

    size_t size;
    char* content = read_file_content(filepath, &size);
    if (!content) return empty_hash;

    SHA1Hash hash = sha1_compute((const uint8_t*)content, size);
    free(content);

    return hash;
}

SHA1Hash hash_commit(const char* message, time_t timestamp,
    const SHA1Hash* parent, const char** files,
    const SHA1Hash* file_hashes, int file_count) {
    size_t buf_size = strlen(message) + 256 + file_count * 512;
    char* buffer = (char*)malloc(buf_size);
    if (!buffer) {
        SHA1Hash empty;
        memset(&empty, 0, sizeof(empty));
        return empty;
    }

    snprintf(buffer, buf_size, "commit %ld\n", (long)timestamp);
    size_t offset = strlen(buffer);

    if (parent) {
        char* parent_str = sha1_to_string(parent);
        offset += snprintf(buffer + offset, buf_size - offset,
            "parent %s\n", parent_str);
        free(parent_str);
    }

    for (int i = 0; i < file_count; i++) {
        char* file_hash_str = sha1_to_string(&file_hashes[i]);
        offset += snprintf(buffer + offset, buf_size - offset,
            "%s %s\n", file_hash_str, files[i]);
        free(file_hash_str);
    }

    SHA1Hash hash = sha1_compute((const uint8_t*)buffer, offset);
    free(buffer);

    return hash;
}