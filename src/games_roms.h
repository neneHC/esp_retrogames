#ifndef _GAMES_ROMS_H_
#define _GAMES_ROMS_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *title;
    const char *subtext;
    const uint8_t *data;
    size_t size;
} nes_game_t;

extern const nes_game_t g_games_list[];
extern const int g_games_count;

extern const uint8_t *selected_rom_data;
extern size_t selected_rom_size;

void select_game(int index);

#ifdef __cplusplus
}
#endif

#endif
