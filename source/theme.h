#include <stdbool.h>
#include <stddef.h>

typedef enum ThemeBase {
	Wii = 'W',
	vWii = 'V',
	Mini = 'M',
} ThemeBase;

typedef struct version_s {
	ThemeBase base;
	char major;
	char minor;
	char region;
} version_t;

typedef enum {
	unknown,
	wiithemer_signed,
	default_theme,
} SignatureLevel;

SignatureLevel SignedTheme(const void* buffer, size_t length);
int InstallTheme(void* buffer, size_t length, int dbpatching);
int DownloadOriginalTheme(void);
int PatchThemeInPlace(void);
int SaveCurrentTheme(void);
version_t GetThemeVersion(const void* buffer, size_t length);
