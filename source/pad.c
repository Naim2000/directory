#include <gctypes.h>
#include <wiiuse/wpad.h>
#include <ogc/pad.h>

static u32 pad_buttons;

void initpads() {
	WPAD_Init();
	PAD_Init();
}

void scanpads() {
	WPAD_ScanPads();
	PAD_ScanPads();
	pad_buttons = WPAD_ButtonsDown(0);

	u16 gcn_down = PAD_ButtonsDown(0);

	if (gcn_down & PAD_BUTTON_A) pad_buttons |= WPAD_BUTTON_A;
	if (gcn_down & PAD_BUTTON_B) pad_buttons |= WPAD_BUTTON_B;
	if (gcn_down & PAD_BUTTON_X) pad_buttons |= WPAD_BUTTON_1;
	if (gcn_down & PAD_BUTTON_Y) pad_buttons |= WPAD_BUTTON_2;
	if (gcn_down & PAD_BUTTON_START) pad_buttons |= WPAD_BUTTON_HOME | WPAD_BUTTON_PLUS;
	if (gcn_down & PAD_BUTTON_UP) pad_buttons |= WPAD_BUTTON_UP;
	if (gcn_down & PAD_BUTTON_DOWN) pad_buttons |= WPAD_BUTTON_DOWN;
	if (gcn_down & PAD_BUTTON_LEFT) pad_buttons |= WPAD_BUTTON_LEFT;
	if (gcn_down & PAD_BUTTON_RIGHT) pad_buttons |= WPAD_BUTTON_RIGHT;
}

void wait_button(u32 button) {
	scanpads();
	while (!(pad_buttons & button /* == button ? */) )
		scanpads();
}

u32 buttons_down() {
	return pad_buttons;
}


