#define _CRT_SECURE_NO_WARNINGS
#include "string_utils.h"
#include <stdarg.h>
#include <ctype.h>

char* str_dup(const char* str) {
	if (!str) return NULL;
	size_t len = strlen(str);
	char* dup = (char*)malloc(len + 1);
	if (dup) {
		memcpy(dup, str, len);
		dup[len] = '\0';
	}
	return dup;
}

char* str_ndup(const char* str, size_t n) {
	if (!str)return NULL;
	size_t len = strlen(str);
	if (n < len)len = n;
	char* dup = (char*)malloc(len + 1);
	if (dup) {
		memcpy(dup, str, len);
		dup[len] = '\0';
	}
	return dup;
}

char* path_join(const char* path1, const char* path2) {
	if (!path1 && !path2) return NULL;
	if (!path1) return str_dup(path2);
	if (!path2) return str_dup(path1);

	size_t len1 = strlen(path1);
	size_t len2 = strlen(path2);

	while (*path2 == '/' || *path2 == '\\') {
		path2++;
		len2--;
	}

	char* result = (char*)malloc(len1 + len2 + 2);
	if (!result) return NULL;

	memcpy(result, path1, len1);
	result[len1] = PATH_SEPARATOR;
	memcpy(result + len1 + 1, path2, len2);
	result[len1 + len2 + 1] = '\0';

	return result;
}

char* path_basename(const char* path) {
	if (!path)return NULL;

	const char* last_sep = NULL;
	const char* p = path;

	while (*p) {
		if (*p == '/' || *p == '\\') {
			last_sep = p;
		}
		p++;
	}

	if (last_sep) {
		return str_dup(last_sep + 1);
	}

	return str_dup(path);
}

bool path_is_absolute(const char* path) {
	if (!path) return false;

#ifdef _WIN32
	if (strlen(path) >= 2 && path[1] == ':') return true;
	if (str_starts_with(path, "\\\\")) return true;
	return false;
#else
	return path[0] == '/';
#endif
}

char* path_normalize(const char* path) {
	if (!path) return NULL;

	char* result = str_dup(path);
	if (!result) return NULL;

	for (char* p = result; *p; p++) {
		if (*p == '\\') *p = '/';
	}

	char* src = result;
	char* dst = result;
	while (*src) {
		if (*src == '/' && *(src + 1) == '/') {
			src++;
		}
		else {
			*dst++ = *src++;
		}
	}
	*dst = '\0';

	size_t len = strlen(result);
	if (len > 1 && result[len - 1] == '/') {
		result[len - 1] = '\0';
	}

	return result;
}

char* path_dirname(const char* path) {
	if (!path) return NULL;

	char* copy = str_dup(path);
	if (!copy) return NULL;

	char* last_sep = NULL;
	char* p = copy;

	while (*p) {
		if (*p == '/' || *p == '\\') {
			last_sep = p;
		}
		p++;
	}

	if (last_sep) {
		if (last_sep == copy) {
			copy[1] = '\0';
		}
		else {
			*last_sep = '\0';
		}
	}
	else {
		copy[0] = '.';
		copy[1] = '\0';
	}

	return copy;
}

char** str_split(const char* str, const char* delimiter, int* count) {
	if (!str || !delimiter || !count) return NULL;

	*count = 0;
	int capacity = 10;
	char** result = (char**)malloc(sizeof(char*) * capacity);
	if (!result) return NULL;

	char* copy = str_dup(str);
	if (!copy) {
		free(result);
		return NULL;
	}

	char* token = strtok(copy, delimiter);
	while (token) {
		if (*count >= capacity) {
			capacity *= 2;
			char** new_result = (char**)realloc(result, sizeof(char*) * capacity);
			if (!new_result) {
				str_free_split(result, *count);
				free(copy);
				return NULL;
			}
			result = new_result;
		}
		
		result[*count] = str_dup(token);
		if (!result[*count]) {
			str_free_split(result, *count);
			free(copy);
			return NULL;
		}
		(*count)++;
		token = strtok(NULL, delimiter);
	}

	free(copy);
	return result;
}

void str_free_split(char** strings, int count) {
	if (!strings) return;
	for (int i = 0; i < count; i++) {
		free(strings[i]);
	}
	free(strings);
}

char* str_trim(char* str) {
	if (!str) return NULL;

	while (isspace((unsigned char)*str)) str++;

	if (*str == 0) return str;

	char* end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	*(end + 1) = '\0';

	return str;
}

bool str_starts_with(const char* str, const char* prefix) {
	if (!str || !prefix) return false;
	return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool str_ends_with(const char* str, const char* suffix) {
	if (!str || !suffix) return false;
	size_t str_len = strlen(str);
	size_t suffix_len = strlen(suffix);
	if (suffix_len > str_len) return false;
	return strcmp(str + str_len - suffix_len, suffix) == 0;
}

char* str_format(const char* format, ...) {
	va_list args;
	va_start(args, format);

	va_list args_copy;
	va_copy(args_copy, args);

	int size = vsnprintf(NULL, 0, format, args_copy);
	va_end(args_copy);

	if (size < 0) {
		va_end(args);
		return NULL;
	}

	char* buffer = (char*)malloc(size + 1);
	if (!buffer) {
		va_end(args);
		return NULL;
	}

	vsnprintf(buffer, size + 1, format, args);
	va_end(args);

	return buffer;
}

void str_tolower(char* str) {
	if (!str) return;

	for (char* p = str; *p; p++) {
		if (*p >= 'A' && *p <= 'Z') {
			*p = *p + ('a' - 'A');
		}
	}
}