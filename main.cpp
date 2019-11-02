#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspctrl.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "fcplay.h"

#define DEBUG 0
#define debug_print(fmt, ...)                  \
	do                                         \
	{                                          \
		if (DEBUG)                             \
			fprintf(stderr, fmt, __VA_ARGS__); \
	} while (0)

PSP_MODULE_INFO("Sine Demo", 0x1000, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define FONT_FILENAME "host0:/assets/chunky.rgba"
#define FONT_SIZE 196608
#define LOGO_FILENAME "host0:/assets/optixx.rgba"
#define LOGO_SIZE 184320
#define LOGO_WIDTH 512
#define LOGO_HEIGHT 90

static unsigned int __attribute__((aligned(16))) list[262144];

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
#define PIXEL_SIZE (4)
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
#define ZBUF_SIZE (BUF_WIDTH SCR_HEIGHT * 2)
#define CHAR_CNT 40

typedef struct
{
	float s, t;
	unsigned int c;
	float x, y, z;
} VERT;

char text[] = {"OPTIXX PSP SINE DEMO $"};

extern int sine_table[];

typedef struct
{
	char *text;
	char *ptr;
	int idx;
	int x;
	int idx_max;
	int idx_step;
	int *table;
	VERT *v;
} stsine;

stsine ssine;

typedef struct
{
	int idx;
	int idx_max;
	int idx_step;
	int *table;
} ltsine;

ltsine lsine;

char *text_block = "TEST 0\n"
				   "TEST 1\n"
				   "TEST 2\n"
				   "TEST 3\n";

typedef struct
{
	int nr;
	int x;
	int y;
	int idx;
	int zoom;
} tblock;

tblock block;

int exit_callback(int arg1, int arg2, void *common)
{
	sceKernelExitGame();
	return 0;
}

int CallbackThread(SceSize args, void *argp)
{
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

unsigned int convert_char(unsigned int c)
{
	if (c == 32)
		c = 47;
	else if (c >= 49 && c <= 57)
		c = c - 23;
	else if (c == 48)
		c = 35;
	else
		c = c - 65;
	return c;
}

void draw_string(const char *text, int x, int y, unsigned int color)
{
	int len = (int)strlen(text);
	VERT *v = (VERT *)sceGuGetMemory(sizeof(VERT) * 2 * len);
	int i;
	for (i = 0; i < len; i++)
	{
		unsigned char c = (unsigned char)text[i];
		c = convert_char(c);
		int fx = (c % 16) * 32;
		int fy = (c / 16) * 32;
		int fw = 32;
		int fh = 32;
		debug_print("i=%02i c=%03i fx=%03i fy=%03i x=%03i y=%03i\n", i, c, fx, fy, x, y);

		VERT *v0 = &v[i * 2 + 0];
		VERT *v1 = &v[i * 2 + 1];

		v0->s = (float)(fx);
		v0->t = (float)(fy);
		v0->c = color;
		v0->x = (float)(x);
		v0->y = (float)(y);
		v0->z = 0.0f;

		v1->s = (float)(fx + fw);
		v1->t = (float)(fy + fh);
		v1->c = color;
		v1->x = (float)(x + fw);
		v1->y = (float)(y + fh);
		v1->z = 0.0f;

		x += fw;
	}

	sceGumDrawArray(GU_SPRITES,
					GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
					len * 2, 0, v);
}

void draw_logo(ltsine *lsine)
{

	lsine->idx += lsine->idx_step;
	if (lsine->idx >= lsine->idx_max)
		lsine->idx -= lsine->idx_max;
	int x = (lsine->table[lsine->idx] / 8);
	VERT *v = (VERT *)sceGuGetMemory(sizeof(VERT) * 2);
	VERT *v0 = &v[+0];
	VERT *v1 = &v[1];

	int tx, ty;
	tx = 0;
	ty = 96;

	v0->s = tx;
	v0->t = ty;
	v0->c = 0xFFFFFFFF;
	v0->x = -512 + x;
	v0->y = 90;
	v0->z = 100.0f;

	v1->t = ty + LOGO_HEIGHT;
	v1->s = tx + LOGO_WIDTH;
	v1->c = 0xFFFFFFFF;
	v1->x = v0->x + LOGO_WIDTH;
	v1->y = v0->y + LOGO_HEIGHT;
	v1->z = 100.0f;

	sceGumDrawArray(GU_SPRITES,
					GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
					2, 0, v);
}

void init_block(tblock *block)
{
	block->x = 1;
	block->y = 1;
	block->idx = 0;
	block->zoom = 32;
}

void init_ssine(stsine *ssine)
{
	ssine->text = text;
	ssine->ptr = ssine->text;
	ssine->idx = 0;
	ssine->x = 32;
	ssine->idx_max = 4095;
	ssine->idx_step = 64;
	ssine->table = sine_table;
}

void init_lsine(ltsine *lsine)
{
	lsine->idx = 0;
	lsine->idx_max = 4095;
	lsine->idx_step = 16;
	lsine->table = sine_table;
}

int draw_block_char(VERT *v, int i, int size, unsigned char c, int x, int y, unsigned int color)
{
	c = convert_char(c);
	int fx = (c % 16) * 32;
	int fy = (c / 16) * 32;
	int fw = 32;
	int fh = 32;

	VERT *v0 = &v[i * 2 + 0];
	VERT *v1 = &v[i * 2 + 1];

	v0->s = (float)(fx);
	v0->t = (float)(fy);
	v0->c = color;
	v0->x = (float)(x)-size;
	v0->y = (float)(y)-size;
	v0->z = 0.0f;

	v1->s = (float)(fx + fw);
	v1->t = (float)(fy + fh);
	v1->c = color;
	v1->x = (float)(v0->x + fw + size * 2);
	v1->y = (float)(v0->y + fh + size * 2);
	v1->z = 0.0f;
	return 0;
}

int draw_sine_char(VERT *v, int i, unsigned char c, int x, int y, unsigned int color)
{

	int idx;
	int z = 0;
	c = convert_char(c);
	int fx = (c % 16) * 32;
	int fy = (c / 16) * 32;
	int fw = 32;
	int fh = 32;

	z = y / 32;

	VERT *v0 = &v[i * 2 + 0];
	VERT *v1 = &v[i * 2 + 1];

	/*
	   if (y + z + fh > 272)
	    y = 272 - z - fh;
	 */

	v0->s = (float)(fx);
	v0->t = (float)(fy);
	v0->c = color;
	v0->x = (float)(x);
	v0->y = (float)(y);
	v0->z = 0.0f;

	v1->s = (float)(fx + fw);
	v1->t = (float)(fy + fh);
	v1->c = color;
	v1->x = (float)(x + fw + z);
	v1->y = (float)(y + fh + z);
	v1->z = 0.0f;
	return (x + fw + z);
}

int draw_block(tblock *block)
{
	int len = strlen(text_block);
	VERT *v = (VERT *)sceGuGetMemory(sizeof(VERT) * 2 * len);
	block->zoom--;
	if (block->zoom == 16)
	{
		block->zoom = 32;
		block->idx++;
		if (block->idx == len)
		{
			block->idx = 0;
		}
	}
	int x = 1;
	int y = 1;
	unsigned char c;
	for (int i = 0; i <= block->idx; i++)
	{
		c = text_block[i];
		if (c == '\n')
		{
			x = 1;
			y++;
			continue;
		}
		x++;
		if (x > 12)
		{
			x = 1;
			y++;
		}
		if (i < block->idx)
			draw_block_char(v, i, 0, c, x * 32, y * 32, 0xFFFFFFFF);
		else
			draw_block_char(v, i, block->zoom, c, x * 32, y * 32, 0xFFFFFFFF);

		debug_print("c=%c x=%i y=%i\n", text_block[i], x * 32, x * 32);
	}
	sceGumDrawArray(GU_SPRITES,
					GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
					(block->idx + 1) * 2, 0, v);
}

void draw_sine(stsine *ssine)
{
	int i, val;
	char *ptr;
	int y;
	int last_x;
	int idx;

	ssine->v = (VERT *)sceGuGetMemory(sizeof(VERT) * 2 * CHAR_CNT);

	ssine->idx += ssine->idx_step;
	if (ssine->idx >= ssine->idx_max)
		ssine->idx = 0;

	ssine->x -= 4;
	if (ssine->x <= -32)
	{
		ssine->x = 0;
		ssine->ptr++;
	}
	ptr = ssine->ptr;
	idx = ((*ptr) - 32);
	val = ssine->idx + (0 * ssine->idx_step);
	if (val >= ssine->idx_max)
		val -= ssine->idx_max;
	y = (ssine->table[ssine->idx_max - val] / 32);

	last_x = ssine->x;
	if (*ssine->ptr == '$')
		ssine->ptr = ssine->text;
	ptr = ssine->ptr;
	for (i = 0; i < CHAR_CNT; i++)
	{
		val = ssine->idx + (i * ssine->idx_step);
		if (val >= ssine->idx_max)
			val -= ssine->idx_max;
		if (*ptr == '$')
			ptr = ssine->text;

		y = (ssine->table[ssine->idx_max - val] / 32);
		last_x = draw_sine_char(ssine->v, i, *ptr, last_x, y, 0xFFFFFFFF);

		ptr++;
	}

	sceGumDrawArray(GU_SPRITES,
					GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
					CHAR_CNT * 2, 0, ssine->v);
}

int main(int argc, char **argv)
{

	SceCtrlData pad, lastpad;
	int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(1);
	init_ssine(&ssine);
	init_lsine(&lsine);

	unsigned char *tex = (unsigned char *)malloc(FONT_SIZE + LOGO_SIZE);
	memset(tex, 0, FONT_SIZE + LOGO_SIZE);
	SceUID fd;
	if ((fd = sceIoOpen(FONT_FILENAME, PSP_O_RDONLY, 0777)))
	{
		sceIoRead(fd, tex, FONT_SIZE);
		sceIoClose(fd);
	}
	fprintf(stderr, "Loaded Data %s %i bytes\n", FONT_FILENAME, FONT_SIZE);
	if ((fd = sceIoOpen(LOGO_FILENAME, PSP_O_RDONLY, 0777)))
	{
		sceIoRead(fd, tex + FONT_SIZE, LOGO_SIZE);
		sceIoClose(fd);
	}
	fprintf(stderr, "Loaded Data %s %i bytes\n", LOGO_FILENAME, LOGO_SIZE);

	fcplay_init();

	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuDrawBuffer(GU_PSM_8888, (void *)0, BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void *)0x88000, BUF_WIDTH);
	sceGuDepthBuffer((void *)0x110000, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
	sceGuDepthRange(0xc350, 0x2710);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);

	sceGuDisable(GU_DEPTH_TEST);
	//sceGuDepthRange(0xc350,0x2710);
	//sceGuEnable(GU_DEPTH_TEST);

	sceGuShadeModel(GU_SMOOTH);

	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, 0, 0, 0);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_DST_COLOR, 0, 0);

	sceGuAlphaFunc(GU_GREATER, 0, 0xff);
	sceGuEnable(GU_ALPHA_TEST);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexImage(0, 512, 256, 512, tex);

	//sceGuTexImage(0, 512, 512, 512, tex);
	//sceGuTexImage(0, 256, 128, 256, tex);

	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexEnvColor(0x0);
	sceGuTexOffset(0.0f, 0.0f);
	//sceGuTexScale(1.0f / 256.0f, 1.0f / 128.0f);
	sceGuTexWrap(GU_REPEAT, GU_REPEAT);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuFinish();
	sceGuSync(0, 0);
	sceGuDisplay(GU_TRUE);
	init_block(&block);
	fcplay_start();
	sceCtrlReadBufferPositive(&lastpad, 1);
	while (1)
	{
		sceCtrlReadBufferPositive(&pad, 1);
		if (pad.Buttons != lastpad.Buttons)
		{
			if (pad.Buttons & PSP_CTRL_CIRCLE)
			{
				break;
			}
			lastpad = pad;
		}

		sceGuStart(GU_DIRECT, list);
		sceGuClear(GU_COLOR_BUFFER_BIT);

		draw_string("SINE SCROLLER", 0, 224, 0x7FFFFFFF);
		draw_block(&block);
		draw_sine(&ssine);
		draw_logo(&lsine);
		sceGuFinish();
		sceGuSync(0, 0);
		//sceKernelDelayThread (50000);
		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
	}

	sceGuDisplay(GU_FALSE);
	sceGuTerm();
	sceKernelExitGame();
	return 0;
}
