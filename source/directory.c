#include "directory.h"

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

static u32 buttons = 0;

struct entry {
	u8 flags;
	char name[NAME_MAX];
};

static char* pwd() {
	static char cwd[PATH_MAX];
	return getcwd(cwd, sizeof(cwd));
}

static inline void cursorpos(int row, int col) {
	printf("\x1b[%d;%dH", row, col);
}

static inline void clear() {
	printf("\x1b[2J");
	fflush(stdout);
}

static inline void scanpads() {
	WPAD_ScanPads();
	buttons = WPAD_ButtonsDown(0);
}

static inline void PrintEntries(struct entry entries[], size_t count, size_t max, size_t selected) {
	size_t cnt = (count > max) ? max : count;
	for (size_t j = 0; j < cnt; j++) {
		if ((selected > (max - 2)) && (j < (selected - (max - 2)))) { cnt++; continue; };
		if (j == selected) printf(">>");
		printf("\t%s\n", entries[j].name);
	}
}

static size_t GetDirectoryEntryCount(DIR* p_dir) {
	size_t count = 0;
	DIR* pdir;
	struct dirent* pent;

	if (!(
		(pdir = p_dir) ||
		(pdir = opendir("."))
	)) return 0;

	while ((pent = readdir(pdir)) != NULL )
		if (!(strequal(pent->d_name, ".") || strequal(pent->d_name, ".."))) count++;

	if(p_dir) rewinddir(pdir);
	else closedir(pdir);
	return count;
}

static size_t ReadDirectory(DIR* p_dir, struct entry entries[], size_t count) {
	DIR* pdir;
	struct dirent *pent;
	struct stat statbuf;
	size_t i = 0;

	if (!(
		(pdir = p_dir) ||
		(pdir = opendir("."))
	)) return 0;

	while (i < count) {
		pent = readdir(pdir);
		if (!pent) break;
		if(strequal(pent->d_name, ".") || strequal(pent->d_name, "..")) continue;

		stat(pent->d_name, &statbuf);
		entries[i].flags = 0x80 | (S_ISDIR(statbuf.st_mode) > 0);
		strcpy(entries[i].name, pent->d_name);
		i++;
	}
	if (!p_dir) closedir(pdir);
	return i;
}

static struct entry* GetDirectoryEntries(struct entry** entries, DIR* p_dir, size_t* count) {
	if (!entries) return NULL;
	DIR* pdir;
	size_t cnt = 0;

	if (!(
		(pdir = p_dir) ||
		(pdir = opendir("."))
	)) return NULL;

	cnt = GetDirectoryEntryCount(pdir);
	if (!cnt) return NULL;

	// If ptr is NULL, then the call is equivalent to malloc(size), for all values of size.
	struct entry* _entries = reallocarray(*entries, cnt, sizeof(struct entry));
	if (!_entries) {
		free(*entries);
		errno = ENOMEM;
		return NULL;
	}
	memset(_entries, 0, sizeof(struct entry) * cnt);
	*entries = _entries;

	*count = ReadDirectory(pdir, *entries, cnt);

	if(p_dir) rewinddir(pdir);
	else closedir(pdir);
	return *entries;
}

char* SelectFileMenu(const char* header) {
	struct entry* entries = NULL;
	int index = 0;
	size_t cnt = 0, max = MAX_ENTRIES;
	static char filename[PATH_MAX];
	char prev_cwd[PATH_MAX];

	if (header) max -= 2;

	if (!getcwd(prev_cwd, sizeof(prev_cwd)))
		perror("Failed to get current working directory?");

	GetDirectoryEntries(&entries, NULL, &cnt);

	for(;;) {
		if (!entries) {
			perror("GetDirectoryEntries failed");
			return NULL;
		}
		clear();
		cursorpos(2, 0);
		if (header) printf("%s\n\n", header);
		printf("Current directory: %s\n\n", pwd());
		PrintEntries(entries, cnt, max, index);

		struct entry* entry = entries + index;
		for(;;) {
			scanpads();
			if (buttons & WPAD_BUTTON_DOWN) {
				if (index < (cnt - 1)) index += 1;
				else index = 0;
				break;
			}
			else if (buttons & WPAD_BUTTON_UP) {
				if (index > 0) index -= 1;
				else index = cnt - 1;
				break;
			}
			else if (buttons & WPAD_BUTTON_A) {
				if (entry->flags & 0x01) {
					chdir(entry->name);
					GetDirectoryEntries(&entries, NULL, &cnt);
					index = 0;
					break;
				}
				else {
					if (filename[sprintf(filename, "%s", pwd()) - 1] != '/') strcat(filename, "/");
					strcat(filename, entry->name);
					chdir(prev_cwd);
					return filename;
				}
			}
			else if (buttons & WPAD_BUTTON_B) {
				if (chdir("..") < 0) {
					if (errno == ENOENT) return NULL;
					else perror("Failed to go to parent dir");
				}
				else {
					GetDirectoryEntries(&entries, NULL, &cnt);
					index = 0;
					break;
				}
			}
		}
	}
}


