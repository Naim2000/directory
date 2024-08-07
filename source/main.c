#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <gccore.h>
#include <ogc/es.h>
#include <wiiuse/wpad.h>
#include <libpatcher.h>

#include "video.h"
#include "malloc.h"
#include "pad.h"
#include "fs.h"
#include "fatMounter.h"
#include "directory.h"
#include "menu.h"
#include "sysmenu.h"
#include "theme.h"
#include "network.h"

__weak_symbol __printflike(1, 2)
void OSReport(const char* fmt, ...) {}

extern void __exception_setreload(int);

static int ww43dbmode = 0, brickprotection = 0;

bool isCSMfile(const char* name) {
	return hasFileExtension(name, ".csm") || hasFileExtension(name, ".app");
}

int SelectTheme() {
	int ret;
	char file[PATH_MAX] = {};
	size_t fsize = 0;
	void* buffer = NULL;

	ret = SelectFileMenu("Select a .csm or .app file.", "/themes", isCSMfile, file);
	clear();

	if (ret) {
	//	extern char cwd[];

	//	printf("GetActiveDeviceName() => %p \"%s\"\n", GetActiveDeviceName(), GetActiveDeviceName());
	//	printf("cwd => %p \"%s\"\n", cwd, cwd);
		perror("SelectFileMenu failed");
		return ret;
	}

	printf("\n%s\n\n", file);

	if (FAT_GetFileSize(file, &fsize) < 0) {
		perror("FAT_GetFileSize failed");
		return -errno;
	}

	printf("File size: %.2fMB\n\n", fsize / (float)(1 << 20));

	printf("Press +/START to install.\n"
			"Press any other button to cancel.\n\n");

	if (!(wait_button(0) & WPAD_BUTTON_PLUS))
		return -ECANCELED;

	buffer = memalign32(fsize);
	if (!buffer) {
		printf("No memory..? (failed to allocate %u bytes)\n", fsize);
		return -ENOMEM;
	}

	ret = FAT_Read(file, buffer, fsize, progressbar);
	if (ret < 0) {
		perror("FAT_Read failed");
		goto finish;
	}

	ret = InstallTheme(buffer, fsize, ww43dbmode == 0);

finish:
	free(buffer);
	return ret;
}

static SettingsItem settings[] = {
	{
		.name = "43DB fix for WC24 channels (vWii)",
		.options = (const char*[]){ "Enabled", "Disabled" },
		.count = 2,
		.selected = &ww43dbmode
	},

	{
		.name = "Brick Protection",
		.options = (const char*[]){ "Enabled", "Active", "Yes", "On", "Offn't" },
		.count = 5,
		.selected = &brickprotection
	}
};

int options(void)
{
	SettingsMenu(settings, 2);
	return 0;
}

static MainMenuItem items[] = {
	{
		.name = "Install a theme",
		.action = SelectTheme,
		.pause = true
	},

	{
		.name = "Apply 43DB fix to current theme (vWii)",
		.action = PatchThemeInPlace,
		.heading = true,
		.pause = true
	},

	{
		.name = "Save current theme",
		.action = SaveCurrentTheme,
		.heading = true,
		.pause = true,
	},

	{
		.name = "Download base theme",
		.action = DownloadOriginalTheme,
		.heading = true,
		.pause = true
	},

	{
		.name = "Options",
		.action = options
	},

	{
		.name = "Exit"
	}
};

int main() {
	__exception_setreload(10);

	puts("Loading...");

	if (!patch_ahbprot_reset() || !patch_isfs_permissions()) {
	//	printf("\x1b[30;1mHW_AHBPROT: %08X\x1b[39m\n", *((volatile uint32_t*)0xcd800064));
		puts("failed to apply IOS patches!\n"
			 "Please make sure that you are running this app on HBC v1.0.8 or later,\n"
			 "and that <ahb_access/> is under the <app> node in meta.xml!\n\n"

			 "Exiting in 5 seconds..."
		);
		sleep(5);
		return *((volatile uint32_t*)0xcd800064);
	}

	initpads();
	ISFS_Initialize();

	if (sysmenu_process() < 0)
		goto waitexit;

	if (!FATMount()) {
		puts("Unable to mount a storage device...");
		goto waitexit;
	}

	if (!sysmenu->hasPriiloader) {
		printf("\x1b[30;1mPlease install Priiloader..!\x1b[39m\n\n");
		sleep(1);

		if (sysmenu->platform == Mini) // There's nooooooooo way you're doing this on Mini with no Priiloader. Illegal
			goto waitexit;

		puts("Press A to continue.");
		wait_button(WPAD_BUTTON_A);
	}

	MainMenu(items, 6);

exit:
	ISFS_Deinitialize();
	FATUnmount();
	network_deinit();
	WPAD_Shutdown();
	return 0;

waitexit:
	puts("Press any button to exit.");
	wait_button(0);
	goto exit;
}
