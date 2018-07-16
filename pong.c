#include <stdint.h>
#include <stddef.h>
#include "random.h"

#include "pong.h"
#include "IO.h"
#include "SoundEffects.dat"

#define GAMEWIDTH 151
#define STARTSPEED_X 4
#define STARTSPEED_Y 1
#define MAXSPEED_X 8
#define MAXSPEED_Y 3


// Local Function Definitions //
void updateIO(void);
void updateDisplay(void);
void DrawCenterline(void);
void DrawBorderLines(void);
void DrawScore(void);
void DrawPaddles(void);
void UpdateBall(void);
void DrawBall(void);
void ResetBall(void);

//Custom datatypes //
typedef struct{
	int16_t x;
	int16_t y;
	int8_t dx;
	int8_t dy;
	Sprite_t * spriteData;
} ball_t;

typedef struct{
	uint8_t playerLeft;
	uint8_t playerRight;
} score_t;


// Local Variables //
knobs_t knobs = {0,0,0};
ball_t ball = {0,0,0,0,0};
score_t score = {0,0};
uint16_t botEdge = 0;
uint16_t topEdge = 0;
uint16_t leftEdge = 0;
uint16_t rightEdge = 0;
uint16_t horizCenter = 0;
uint16_t vertCenter = 0;


SoundEffect_t *wallBounce = &(SoundEffect_t){PONG_WALL,256};
SoundEffect_t *paddleBounce = &(SoundEffect_t){PONG_PADDLE, 1536};
SoundEffect_t *paddleMiss = &(SoundEffect_t){PONG_MISS, 4112};


const uint8_t paddleSprite[12] = {0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0};
const uint8_t ballSprite[] = {0xF0,0xF0,0xF0,0xF0};
const uint8_t verticalLineSprite[4] = {0x80,0x80,0x80,0x80};

Sprite_t paddleData 			= {.widthBits = 3, .heightBits = 12, .sprite = paddleSprite};
Sprite_t ballData 				= {.widthBits = 4, .heightBits = 4,  .sprite = ballSprite};
Sprite_t verticalLineData = {.widthBits = 1, .heightBits = 4,  .sprite = verticalLineSprite};


// Functions //

void Pong_rand(void){
	updateIO();
	Random_Init(knobs.left+knobs.right);
}

void Pong_Init(void){
	score.playerLeft = 0;
	score.playerRight = 0;
	ResetBall();
}

void ResetBall(void){
	uint16_t screenWidth = IO_getScreenWidth();
	uint16_t screenHeight = IO_getScreenHeight();

	topEdge = 0;
	botEdge = screenHeight;
	horizCenter = screenWidth/2;
	vertCenter = screenHeight/2;
	leftEdge = horizCenter - (GAMEWIDTH-1)/2;
	rightEdge = horizCenter + (GAMEWIDTH-1)/2;
	
	int8_t startingSign[] = {1,-1};
	ball.x = horizCenter;
	ball.y = ((Random32()>>24)%vertCenter)+vertCenter/2;
	ball.dx = STARTSPEED_X*startingSign[(Random32()>>24)%2];
	ball.dy = STARTSPEED_Y*startingSign[(Random32()>>24)%2];
	ball.spriteData = &ballData;
}

void Pong_UpdateGame(void){
	updateIO();
	updateDisplay();
}

void updateIO(void){
	knobs = IO_ReadKnobs();
	
	// clockwise = higher on screen
	knobs.left = (knobs.left*(botEdge-paddleData.heightBits)/knobs.max);
	
	// clockwise = lower on screen
	knobs.right = (botEdge-paddleData.heightBits)-(knobs.right*(botEdge-paddleData.heightBits)/knobs.max);	
}

void updateDisplay(void){
	DrawCenterline();
	DrawBorderLines();
	DrawScore();
	DrawPaddles();
	UpdateBall();
	DrawBall();
}

void DrawCenterline(void){
	unsigned short xPos = ((rightEdge-leftEdge)>>1) + leftEdge;
	int i;
	for(i=0;i<(botEdge);i=i+10){
		IO_LoadSprite(xPos,i,verticalLineData);
	}
}

void DrawBorderLines(void){
	unsigned short leftPos = leftEdge;
	unsigned short rightPos = rightEdge;
	int i;
	for(i=0;i<(botEdge);i=i+4){
		IO_LoadSprite(leftPos,i,verticalLineData);
		IO_LoadSprite(rightPos,i,verticalLineData);
	}
}

void DrawScore(void){
	IO_PrintNumber(horizCenter-20,5,score.playerLeft);
	IO_PrintNumber(horizCenter+12,5,score.playerRight);
}

void DrawPaddles(void){
	IO_LoadSprite((unsigned short)leftEdge+2,(unsigned short)knobs.left,paddleData);
	IO_LoadSprite((unsigned short)rightEdge-4,(unsigned short)knobs.right,paddleData);
}

void UpdateBall(void){
	ball.x += ball.dx;
	ball.y += ball.dy;
	
	//Goes off left edge
	if(ball.x < leftEdge-1)
	{
		IO_PlaySound(paddleMiss);
		score.playerRight += 1;
		ResetBall();
	}
	
	//Goes off right edge
	if(ball.x > rightEdge)
	{
		IO_PlaySound(paddleMiss);
		score.playerLeft += 1;
		ResetBall();
	}
	
	//Hits top of screen
	if(ball.y < 0)
	{
		ball.y = 1;
		ball.dy = -ball.dy;
		ball.dy = ball.dy+1<MAXSPEED_Y ? ball.dy+1 : MAXSPEED_Y;
		IO_PlaySound(wallBounce);
	}
	
	//Hits bottom of screen
	if(ball.y > botEdge-ballData.heightBits)
	{
		ball.y = botEdge-ballData.heightBits-1;
		ball.dy = -ball.dy;
		ball.dy = ball.dy-1>-MAXSPEED_Y ? ball.dy-1 : -MAXSPEED_Y;
		IO_PlaySound(wallBounce);
	}
	
	if(	(ball.x >= rightEdge-4-4-ball.dx) && 
			(ball.y	>	 knobs.right-4) && 
			(ball.y	<	 knobs.right+paddleData.heightBits) )
	{
		ball.x = rightEdge-8;
		ball.dx = -ball.dx;
		ball.dx = ball.dx-1>-MAXSPEED_X ? ball.dx-1 : -MAXSPEED_X;
		IO_PlaySound(paddleBounce);
	}
	
	if(	(ball.x < leftEdge+4-ball.dx) && 
			(ball.y	>	knobs.left-4) && 
			(ball.y	<	knobs.left+paddleData.heightBits) )
	{
		ball.x = leftEdge+5;
		ball.dx = -ball.dx;
		ball.dx = ball.dx+1<MAXSPEED_X ? ball.dx+1 : MAXSPEED_X;
		IO_PlaySound(paddleBounce);
	}
	
	
}

void DrawBall(void){
	IO_LoadSprite(ball.x,ball.y,*(ball.spriteData));
}
