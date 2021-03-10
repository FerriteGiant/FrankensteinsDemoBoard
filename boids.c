#include "random.h"
#include "IO.h"

#define SCREEN_H 64
#define SCREEN_W 480

#define boidNum   100

#define COM_multiplier        130
#define AVOID_multiplier      25
#define MATCHVEL_multiplier   120
#define friendRadiusSqr       121
#define repelBoidDistSqr      81

const uint8_t boidSprite[3] = {0xF0,0xF0,0xF0};
Sprite_t boidData = {3,3,boidSprite};

long abs_long(long a){
  return (a>=0) ? (a) : (-1*a);
}

long min_long(long a, long b){
  return (a<=b) ? (a) : (b);
}

long nonzero_random(long max){
  long result = 0;
  while(result==0){
    result = (Random32()>>21)%(max*2+1) - max;
  }
  return result;
}


struct State {
  long x;      // x coordinate
  long y;      // y coordinate
  long vx;
  long vy;
  //const unsigned char *image; // ptr->image
};  
typedef struct State State_t;

struct Vector{
  long x;
  long y;
};
typedef struct Vector Vector_t;

struct FriendInfo{
  unsigned char iBoids;
  unsigned short distSqr;
};
typedef struct FriendInfo Friend_t;


State_t boids[boidNum];
Friend_t friends[boidNum-1];


/// INITIALIZE BOIDS
void Boids_Init(void){
  Random_Init(17957); //Arbitrary seed
  for(size_t i=0; i<boidNum; i++){
    boids[i].x = ((i*13)+100)%(SCREEN_W-boidData.widthBits);
    boids[i].y = (10+Random32()%40)%(SCREEN_H-boidData.heightBits);
    boids[i].vx = nonzero_random(3)*100;
    boids[i].vy = nonzero_random(3)*100;
    //boids[i].image = boidData.spriteData;
  }
}

unsigned long friends_DATA(unsigned long b, long* xCOM, long* yCOM, long* vxAVG, long* vyAVG){
  unsigned long numFriends=0;
  long xSum=0;
  long ySum=0;
  long vxSum=0;
  long vySum=0;

  unsigned short boidDistSqr;

  for(size_t i=0;i<boidNum;i++){
    if(i==b){continue;}
    boidDistSqr = ((boids[i].x-boids[b].x)*(boids[i].x-boids[b].x)) + ((boids[i].y-boids[b].y)*(boids[i].y-boids[b].y));
    if(boidDistSqr <= friendRadiusSqr){
      friends[numFriends].iBoids = i;
      friends[numFriends].distSqr = boidDistSqr;
      numFriends++;
      xSum += boids[i].x;
      ySum += boids[i].y;
      vxSum += boids[i].vx;
      vySum += boids[i].vy;
    }
  }
  *xCOM = xSum*100/numFriends;
  *yCOM = ySum*100/numFriends;
  *vxAVG = vxSum/numFriends;
  *vyAVG = vySum/numFriends;
  return numFriends;

}

///// TEND TOWARD CENTER OF MASS
Vector_t rule_COM(long xCOM, long yCOM, unsigned long b){
  Vector_t velVector;
  velVector.x = (xCOM-boids[b].x*100);
  velVector.y = (yCOM-boids[b].y*100);
  return velVector;
}

///// AVOID NEIGHBORS AND OBSTICALS
Vector_t rule_AVOID(unsigned long b,unsigned long numFriends){
  Vector_t velVector = {0,0};

  unsigned char repelObsDist = 9; //
  for(size_t i=0;i<numFriends;i++){
    unsigned char iBoids = friends[i].iBoids;
    if(friends[i].distSqr <= repelBoidDistSqr){
      velVector.x = velVector.x - (boids[iBoids].x-boids[b].x);
      velVector.y = velVector.y - (boids[iBoids].y-boids[b].y);       
    }
  }

  if(boids[b].x<repelObsDist){ //to close to left
    velVector.x = velVector.x + (abs(boids[b].x)+1)*3;
  }
  else if(boids[b].x>(SCREEN_W-repelObsDist)){ //to close to right
    velVector.x = velVector.x - (abs(SCREEN_W-boids[b].x)-1)*3;
  }

  if(boids[b].y<repelObsDist){ //to close to top
    velVector.y = velVector.y + (abs(boids[b].y)+1)*3;
  }
  else if(boids[b].y>(SCREEN_H-repelObsDist)){ //to close to bottom
    velVector.y = velVector.y - (abs(SCREEN_H-boids[b].y)+1)*3;
  }

  velVector.x = velVector.x*100;
  velVector.y = velVector.y*100;
  return velVector;
}

///// TEND TOWARD NEIGHBOR VELOCITY
Vector_t rule_MATCHVEL(long vxAvg, long vyAvg, unsigned long b){
  Vector_t velVector = {0,0};
  velVector.x = (vxAvg-boids[b].vx)*100/abs(vxAvg-boids[b].vx);
  velVector.y = (vyAvg-boids[b].vy)*100/abs(vyAvg-boids[b].vy);
  return velVector;
}

void Boids_update(void){
  long xCOM,yCOM,vxAvg,vyAvg;
  for(size_t b=0;b<boidNum;b++){
    unsigned long numFriends = friends_DATA(b,&xCOM, &yCOM, &vxAvg, &vyAvg);
    Vector_t vel = {0,0};
    Vector_t v1 = rule_COM(xCOM,yCOM,b);
    Vector_t v2 = rule_AVOID(b,numFriends);
    Vector_t v3 = rule_MATCHVEL(vxAvg,vyAvg,b);

    v1.x *= COM_multiplier;
    v1.y *= COM_multiplier;
    v2.x *= AVOID_multiplier;
    v2.y *= AVOID_multiplier;
    v3.x *= MATCHVEL_multiplier;
    v3.y *= MATCHVEL_multiplier;

    vel.x = min_long(((long)(boids[b].vx*1.2) + v1.x/100 + v2.x/100 + v3.x/100),400);
    vel.y = min_long(((long)(boids[b].vy*1.2) + v1.y/100 + v2.y/100 + v3.y/100),400);

    boids[b].x = ((boids[b].x*100+vel.x)/100);
    boids[b].y = ((boids[b].y*100+vel.y)/100);
    boids[b].vx = vel.x;
    boids[b].vy = vel.y;
    if(boids[b].x<=0){
      boids[b].x=1;
      boids[b].vx = -boids[b].vx;
    }
    if(boids[b].y<=0){
      boids[b].y=1;
      boids[b].vy= -boids[b].vy;
    }
    if(boids[b].x>=SCREEN_W-2){
      boids[b].x=SCREEN_W-1;
      boids[b].vx= -boids[b].vx;
    }
    if(boids[b].y>=SCREEN_H-2){
      boids[b].y=SCREEN_H-1;
      boids[b].vy= -boids[b].vy;
    }
    if(boids[b].vx==0 && boids[b].vy==0){
      boids[b].vx=nonzero_random(4)*100;
      boids[b].vy=nonzero_random(4)*100;
    }
    //    if(boids[b].vx<100){
    //      boids[b].vx = (((Random32()>>21)%301)-150);
    //    }
    //    if(boids[b].vy<100){
    //      boids[b].vy = (((Random32()>>21)%301)-150);
    //    }
  }

}

void Boids_load(void){
  for(size_t i=0;i<boidNum;i++){ 
    IO_LoadSprite(boids[i].x,boids[i].y,boidData);
  }
}

