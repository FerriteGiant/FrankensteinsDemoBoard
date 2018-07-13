#include <stdint.h>
#include <stdbool.h>


typedef struct {
	uint16_t left;
	uint16_t right;
	uint16_t max;
} knobs_t;

typedef struct
{
	const uint8_t widthBits;
	const uint8_t heightBits;
	const uint8_t * sprite;

} Sprite_t;


typedef struct{
	const uint16_t * waveForm;
	const uint32_t len;
} SoundEffect_t;

void IO_Init(void);
bool IO_Ready(void);
void IO_UpdatesCompleted(void);
uint16_t IO_getScreenWidth(void);
uint16_t IO_getScreenHeight(void);
void IO_PlaySound(SoundEffect_t * sound);
void IO_ToggleMute(void);
void IO_PrintMuteStatus(uint8_t xpos, uint8_t ypos);
bool IO_GetLeftButtonState(void);
bool IO_GetCenterButtonState(void);
bool IO_GetRightButtonState(void);
knobs_t IO_ReadKnobs(void);
void IO_PrintString(uint16_t xpos, uint8_t ypos, const char * string);
void IO_PrintNumber(uint16_t xpos, uint8_t ypos, uint16_t num);
void IO_LoadSprite(	const uint16_t xpos, 
										const uint16_t ypos, 
										const Sprite_t sprite );
