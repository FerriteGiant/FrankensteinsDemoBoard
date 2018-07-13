#ifndef SCREEN_H_
#define SCREEN_H_

#include <stdint.h>
#include <stdbool.h>
#include <iso646.h>
#include <stddef.h>

#define	SCREEN_W 		480
#define	SCREEN_H			64

void Screen_Init(void);
void Screen_SetBufferIsFull(void);
bool Screen_IsBufferAvailable(void);
void Screen_ClearBuffer(void);
uint8_t * Screen_GetBuffer(void);

//void Screen_FrameClear(void);

#endif /* SCREEN_H_ */

