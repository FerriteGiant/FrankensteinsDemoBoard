#include "DisplayTests.h"
#include "IO.h"

//#define PE1 (*((volatile unsigned long *)0x40024008))

const uint8_t verticalWall[8] = {0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,};
const uint8_t horizontalWall[1] = {0xFF};
const uint8_t angledWall[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

Sprite_t vertWallData		= {.widthBits = 1, .heightBits = 8, .sprite = verticalWall};
Sprite_t horizWallData	= {.widthBits = 8, .heightBits = 1, .sprite = horizontalWall};
Sprite_t angledWallData	= {.widthBits = 8, .heightBits = 8, .sprite = angledWall};


void DisplayTests_DrawBorder(void){
	uint16_t screenWidth = IO_getScreenWidth();
	uint16_t screenHeight = IO_getScreenHeight();
	for(uint16_t i=0;i<(screenWidth);i=i+12){
		IO_LoadSprite(i,0,horizWallData);
		IO_LoadSprite(i,screenHeight-1,horizWallData);
		if(i==228){i=i+4;}
	}

	for(uint16_t i=0;i<(screenHeight);i=i+14){
		IO_LoadSprite(0,i,vertWallData);
		IO_LoadSprite(screenWidth-1,i,vertWallData);
	}
}

void DisplayTests_DrawDiag(void){
	uint16_t screenWidth = IO_getScreenWidth();
	for(uint16_t i=0;i<=screenWidth-8;i+=8){
		IO_LoadSprite(i,i%64,angledWallData);
		}
}





//for(i=0;i<20;i++){
//	IO_LoadSprite(225+2*i,15,verticalWall);
//}
//IO_LoadSprite(x,11,block);
//if(x>255) vel=-1;
//if(x<225) vel=+1;
//x = x+vel;

