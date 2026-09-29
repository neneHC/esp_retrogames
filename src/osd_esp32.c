#include <string.h>
#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <esp_heap_caps.h>
#include <esp_timer.h>

#include <noftypes.h>
#include <nofrendo.h>
#include <event.h>
#include <gui.h>
#include <log.h>
#include <nes/nes.h>
#include <nes/nes_pal.h>
#include <nes/nesinput.h>
#include <nofconfig.h>
#include <osd.h>

TimerHandle_t timer;

/* Alocação de memória */
void *mem_alloc(int size, bool prefer_fast_memory)
{
	void *ptr = malloc(size);
	if (!ptr)
	{
		ptr = heap_caps_malloc(size, MALLOC_CAP_DEFAULT);
	}
	return ptr;
}

/* Áudio (stubs para desempenho máximo e estabilidade da emulação) */
int osd_init_sound() { return 0; }
void osd_stopsound() {}
void do_audio_frame() {}
void osd_setsound(void (*playfunc)(void *buffer, int length)) {}
void osd_getsoundinfo(sndinfo_t *info)
{
	if (info)
	{
		info->sample_rate = 0;
		info->bps = 16;
	}
}

/* Display */
extern void display_init();
extern void display_write_frame(const uint8_t *data[]);
extern void display_clear();

static char fb[1]; // dummy
bitmap_t *myBitmap = NULL;

static int init(int width, int height)
{
	display_init();
	if (!myBitmap)
	{
		myBitmap = bmp_createhw((uint8 *)fb, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, NES_SCREEN_WIDTH * 2);
	}
	return 0;
}

static void shutdown(void)
{
	if (myBitmap)
	{
		bmp_destroy(&myBitmap);
		myBitmap = NULL;
	}
}

static int set_mode(int width, int height) { return 0; }

uint16_t myPalette[256];
static void set_palette(rgb_t *pal)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		uint16_t r = (pal[i].r >> 3) & 0x1F;
		uint16_t g = (pal[i].g >> 2) & 0x3F;
		uint16_t b = (pal[i].b >> 3) & 0x1F;
		uint16_t c = (r << 11) | (g << 5) | b;
		// Pré-inverte bytes para envio direto via SPI MSB no ST7789
		myPalette[i] = (c >> 8) | ((c & 0xFF) << 8);
	}
}

static void clear(uint8 color)
{
	display_clear();
}

static bitmap_t *lock_write(void)
{
	return myBitmap;
}

static void free_write(int num_dirties, rect_t *dirty_rects)
{
}

static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects)
{
	if (bmp && bmp->line)
	{
		display_write_frame((const uint8_t **)bmp->line);
	}
}

viddriver_t sdlDriver = {
	"ST7789_Driver",
	init,
	shutdown,
	set_mode,
	set_palette,
	clear,
	lock_write,
	free_write,
	custom_blit,
	false
};

void osd_getvideoinfo(vidinfo_t *info)
{
	info->default_width = NES_SCREEN_WIDTH;
	info->default_height = NES_SCREEN_HEIGHT;
	info->driver = &sdlDriver;
}

/* Controle */
extern void controller_init();
extern uint32_t controller_read_input();

static void osd_initinput()
{
	controller_init();
}

static void osd_freeinput(void) {}

void osd_getinput(void)
{
	const int ev[32] = {
		event_joypad1_up, event_joypad1_down, event_joypad1_left, event_joypad1_right,
		event_joypad1_select, event_joypad1_start, event_joypad1_a, event_joypad1_b,
		event_state_save, event_state_load, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0};
	static int oldb = 0xffff;
	uint32_t b = controller_read_input();
	uint32_t chg = b ^ oldb;
	int x;
	oldb = b;
	event_t evh;

	for (x = 0; x < 16; x++)
	{
		if (chg & 1)
		{
			evh = event_get(ev[x]);
			if (evh)
				evh((b & 1) ? INP_STATE_BREAK : INP_STATE_MAKE);
		}
		chg >>= 1;
		b >>= 1;
	}
}

void osd_getmouse(int *x, int *y, int *button) {}

static int logprint(const char *string)
{
	return printf("%s", string);
}

int osd_init()
{
	nofrendo_log_chain_logfunc(logprint);
	display_init();
	osd_initinput();
	return 0;
}

void osd_shutdown()
{
	osd_freeinput();
}

char configfilename[] = "na";
int osd_main(int argc, char *argv[])
{
	config.filename = configfilename;
	return main_loop(argv[0], system_autodetect);
}

static esp_timer_handle_t s_nes_timer = NULL;
static void nes_timer_callback(void* arg)
{
	nofrendo_ticks++;
}

int osd_installtimer(int frequency, void *func, int funcsize, void *counter, int countersize)
{
	if (s_nes_timer)
	{
		esp_timer_stop(s_nes_timer);
		esp_timer_delete(s_nes_timer);
		s_nes_timer = NULL;
	}

	const esp_timer_create_args_t timer_args = {
		.callback = &nes_timer_callback,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "nes_timer"
	};

	esp_err_t err = esp_timer_create(&timer_args, &s_nes_timer);
	if (err == ESP_OK)
	{
		esp_timer_start_periodic(s_nes_timer, 1000000 / frequency);
		printf("[TIMER] Hardware timer ativo a %d Hz (%d us)\n", frequency, 1000000 / frequency);
		return 0;
	}
	return -1;
}

void osd_fullname(char *fullname, const char *shortname)
{
	strncpy(fullname, shortname, PATH_MAX);
}

char *osd_newextension(char *string, char *ext)
{
	size_t l = strlen(string);
	if (l >= 3)
	{
		string[l - 3] = ext[1];
		string[l - 2] = ext[2];
		string[l - 1] = ext[3];
	}
	return string;
}

int osd_makesnapname(char *filename, int len)
{
	return -1;
}
