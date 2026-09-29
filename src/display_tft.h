#ifndef _DISPLAY_TFT_H_
#define _DISPLAY_TFT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void display_begin();
void display_init();
void display_write_frame(const uint8_t *data[]);
void display_clear();

#ifdef __cplusplus
}
#endif

#endif
