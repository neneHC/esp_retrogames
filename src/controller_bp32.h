#ifndef _CONTROLLER_BP32_H_
#define _CONTROLLER_BP32_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void controller_init();
uint32_t controller_read_input();
void start_bluetooth_task();
bool controller_is_connected();
void set_game_running(bool running);

#ifdef __cplusplus
}
#endif

#endif
