#include <string.h>
#include <stdbool.h>
#include <gctypes.h>

#define MAX_ENTRIES 20

typedef bool (*FileFilter)(const char* name);

char* pwd();
char* SelectFileMenu(const char* header, const char* defaultFolder, FileFilter filter);

static inline const char* fileext(const char* name) {
	if ((name = strrchr(name, '.'))) name += 1;
	return name;
}

static inline bool strequal(const char* a, const char* b) {
	return (strcmp(a, b) == 0) && (strlen(a) == strlen(b));
}
