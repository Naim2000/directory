#include "theme.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <gctypes.h>
#include <ogc/es.h>
#include <mbedtls/sha1.h>
#include <mbedtls/aes.h>

#include "malloc.h"
#include "sysmenu.h"
#include "fs.h"
#include "network.h"

static const char
	wiithemer_sig[] = "wiithemer",
	binary_path_search[] = "C:\\Revolution\\ipl\\System",
	binary_path_search_2[] = "D:\\Compat_irdrepo\\ipl\\Compat",
	binary_path_search_3[] = "c:\\home\\neceCheck\\WiiMenu\\ipl\\bin\\RVL\\Final_", // What
	binary_path_fmt12[] = "%c_%c\\ipl\\bin\\RVL\\Final_%c",
	u8_header[] = { 0x55, 0xAA, 0x38, 0x2D };

// ?
extern void* memmem(const void* haystack, size_t haystack_len, const void* needle, size_t needle_len);

SignatureLevel SignedTheme(const void* buffer, size_t length) {
	sha1 hash = {};
	mbedtls_sha1_ret(buffer, length, hash);

	if (isArchive(hash))
		return default_theme;
	else if (memmem(buffer, length, wiithemer_sig, strlen(wiithemer_sig)))
		return wiithemer_signed;

	else
		return unknown;
}

version_t GetThemeVersion(const void* buffer, size_t length) {
	ThemeBase base;
	char rgn = 0, major = 0, minor = 0;
	char* ptr;


	if ((ptr = memmem(buffer, length, binary_path_search, strlen(binary_path_search)))) {
		base = Wii;
		ptr += strlen(binary_path_search);
	}

	else if ((ptr = memmem(buffer, length, binary_path_search_2, strlen(binary_path_search_2)))) {
		base = vWii;
		ptr += strlen(binary_path_search_2);
	}

	else if ((ptr = memmem(buffer, length, binary_path_search_3, strlen(binary_path_search_3)))) { // Wii mini bruh
		ptr += strlen(binary_path_search_3);
		rgn = *ptr;
		return (version_t) { Mini, '4', '3', rgn };
	}

	if (!ptr)
		return (version_t) { '?', '?', '?', '?' };

	sscanf(ptr, binary_path_fmt12, &major, &minor, &rgn);
	return (version_t) { base, major, minor, rgn };
}

int InstallTheme(const void* buffer, size_t size) {
	if (memcmp(buffer, "PK", 2) == 0) {
		puts("\x1b[31;1mPlease do not rename .mym files.\x1b[39m\n"
			"Follow https://wii.hacks.guide/themes to properly convert it."
		);
		return -EINVAL;
	}
	else if (memcmp(buffer, u8_header, sizeof(u8_header)) != 0) {
		puts("\x1b[31;1mNot a theme!\x1b[39m");
		return -EINVAL;
	}

	version_t themeversion = GetThemeVersion(buffer, size);
	if (themeversion.region != getSmRegion() || themeversion.major != getSmVersionMajor() || themeversion.base != getSmPlatform()) {
		printf("\x1b[41;30mIncompatible theme!\x1b[40;39m\n"
			   "Theme version : %c %c.X%c\n"
			   "System version: %c %c.X%c\n", themeversion.base, themeversion.major,  themeversion.region,
											  getSmPlatform(),   getSmVersionMajor(), getSmRegion());
		return -EINVAL;
	}

	switch (SignedTheme(buffer, size)) {
		case default_theme:
			puts("\x1b[32;1mThis is the default theme for your Wii menu.\x1b[39m");
			break;
		case wiithemer_signed:
			puts("\x1b[36;1mThis theme was signed by wii-themer.\x1b[39m");
			break;

		default:
			puts("\x1b[30;1mThis theme isn't signed...\x1b[39m");
			if (!hasPriiloader()) {
				puts("Consider installing Priiloader before installing unsigned themes.");
				return -EPERM;
			}
			break;
	}

	char filepath[ISFS_MAXPATH] = {};
	sprintf(filepath, "/title/00000001/00000002/content/%08x.app", getArchiveCid());
	printf("%s\n", filepath);
	int ret = NAND_Write(filepath, buffer, size, progressbar);
	if (ret < 0) {
		printf("error! (%d)\n", ret);
		return ret;
	}

	return 0;
}

int DownloadOriginalTheme(bool silent) {
	int ret;
	char url[0x80] = "http://nus.cdn.shop.wii.com/ccs/download/";
	char filepath[ISFS_MAXPATH];
	size_t fsize = getArchiveSize();
	blob download = {};
	mbedtls_aes_context title = {};
	aeskey iv = { 0x00, 0x01 };

	if (!silent) puts("Downloading original theme.");

	void* buffer = memalign32(fsize);
	if (!buffer) {
		printf("No memory...??? (failed to allocate %u bytes)\n", fsize);
		return -ENOMEM;
	}

	sprintf(filepath, "/title/00000001/00000002/content/%08x.app", getArchiveCid());
	ret = NAND_Read(filepath, buffer, fsize, NULL);

	sprintf(filepath, "%08x-v%hu.app", getArchiveCid(), getSmVersion());
	if (ret >= 0 && (SignedTheme(buffer, fsize) == default_theme)) goto save;

	ret = FAT_Read(filepath, buffer, fsize, NULL);
	if (ret >= 0 && (SignedTheme(buffer, fsize) == default_theme)) {
		if (!silent) printf("Already saved. Look for the %s file.\n", filepath);
		goto finish;
	}

	puts("Initializing network... ");
	ret = network_init();
	if (ret < 0) {
		printf("Failed to intiailize network! (%d)\n", ret);
		goto finish;
	}

	puts("Downloading...");
	sprintf(strrchr(url, '/'), "/%016llx/%08x", getSmNUSTitleID(), getArchiveCid());
	ret = DownloadFile(url, &download);
	if (ret != 0) {
		printf(
			"Download failed! (%d)\n"
			"Error details:\n"
			"	%s\n", ret, GetLastDownloadError());

		goto finish;
	}

	puts("Decrypting...");
	mbedtls_aes_setkey_dec(&title, getSmTitleKey(), 128);
	mbedtls_aes_crypt_cbc(&title, MBEDTLS_AES_DECRYPT, download.size, iv, download.ptr, buffer);
	free(download.ptr);

save:
	if (!silent || FAT_GetFileSize(filepath, NULL) < 0) {
		puts("Saving...");
		ret = FAT_Write(filepath, buffer, fsize, progressbar);
		if (ret < 0)
			perror("Failed to save original theme");
		else
			printf("Saved original theme to %s.\n", filepath);
	}

finish:
	free(buffer);
	sleep(3);
	return ret;
}
