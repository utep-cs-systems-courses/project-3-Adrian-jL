#include <stdio.h>
#include <stdlib.h>
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6


typedef struct MovLayer_s{
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

char *scoreText = "0-0";
u_int switches;
char scoreOne = 0;
char scoreTwo = 0;
u_int bgColor = COLOR_BLACK;
int redrawScreen = 1;

AbRect paddleOneShape = {abRectGetBounds, abRectCheck, {8, 2}};
AbRect paddleTwoShape = {abRectGetBounds, abRectCheck, {8, 2}};

Layer paddleOneLayer = {(AbShape*)&paddleOneShape, {6, 5}, {0, 0}, {6, 5}, COLOR_WHITE, 0};
Layer paddleTwoLayer = {(AbShape*)&paddleTwoShape, {120, 155}, {0, 0}, {120, 155}, COLOR_WHITE,
                        &paddleOneLayer};
Layer ballLayer = {(AbShape*)&circle6, {30, 30}, {0, 0}, {0, 0}, COLOR_RED, &paddleTwoLayer};

MovLayer paddleOne = {&paddleOneLayer, {0, 0}, 0};
MovLayer paddleTwo = {&paddleTwoLayer, {0, 0}, &paddleOne};
MovLayer ball = {&ballLayer, {3, 3}, &paddleTwo};


void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next){
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);

  for (movLayer = movLayers; movLayer; movLayer = movLayer->next){
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1],
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}

char hitPaddle(Region *ballBounds, Region *paddleOneBounds, Region *paddleTwoBounds)
{
  char pOneXValCheck = (ballBounds->topLeft.axes[0] >= paddleOneBounds->topLeft.axes[0] && ballBounds->topLeft.axes[0] <= paddleOneBounds->botRight.axes[0]) || (ballBounds->botRight.axes[0] >= paddleOneBounds->topLeft.axes[0] && ballBounds->botRight.axes[0] <= paddleOneBounds->botRight.axes[0]);
  
  char pOneYValCheck = (ballBounds->topLeft.axes[1] >= paddleOneBounds->topLeft.axes[1] && ballBounds->topLeft.axes[1] <= paddleOneBounds->botRight.axes[1]);
  
    //|| (ballBounds->botRight.axes[1] >= paddleOneBounds->topLeft.axes[1] && ballBounds->botRight.axes[1] <= paddleOneBounds->botRight.axes[1]);
  
  char pTwoXValCheck = (ballBounds->topLeft.axes[0] >= paddleTwoBounds->topLeft.axes[0] && ballBounds->topLeft.axes[0] <= paddleTwoBounds->botRight.axes[0]) || (ballBounds->botRight.axes[0] >= paddleTwoBounds->topLeft.axes[0] && ballBounds->botRight.axes[0] <= paddleTwoBounds->botRight.axes[0]);

  char pTwoYValCheck = (ballBounds->botRight.axes[1] >= paddleTwoBounds->topLeft.axes[1] && ballBounds->botRight.axes[1] <= paddleTwoBounds->botRight.axes[1]);
  
    // || (ballBounds->botRight.axes[1] >= paddleTwoBounds->topLeft.axes[1] && ballBounds->botRight.axes[1] <= paddleTwoBounds->botRight.axes[1]);

  return (pOneXValCheck && pOneYValCheck) || (pTwoXValCheck && pTwoYValCheck);
}

char playSound = 0;

//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void ballAdvance(MovLayer *ml)
{
  Vec2 newPos;
  u_char axis;
  Region ballBounds;
  
  vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
  abShapeGetBounds(ml->layer->abShape, &newPos, &ballBounds);
  Region paddleOneBounds, paddleTwoBounds;
  abShapeGetBounds(ml->layer->abShape, &newPos, &ballBounds);
  abShapeGetBounds(paddleOneLayer.abShape, &paddleOneLayer.pos, &paddleOneBounds);
  abShapeGetBounds(paddleTwoLayer.abShape, &paddleTwoLayer.pos, &paddleTwoBounds);
// layerGetBounds(&paddleOneLayer, &paddleOneBounds);
//  layerGetBounds(&paddleTwoLayer, &paddleTwoBounds);
  
  if (hitPaddle(&ballBounds, &paddleOneBounds, &paddleTwoBounds)){
    int velocity = ml->velocity.axes[1] = -ml->velocity.axes[1];
    newPos.axes[1] += (2 * velocity);
    playSound = 1;
  }

  if ((ballBounds.topLeft.axes[0] <= 0) || (ballBounds.botRight.axes[0] >= 127)){
      int velocity = ml->velocity.axes[0] = -ml->velocity.axes[0];
      newPos.axes[0] += (2 * velocity);
      playSound = 1;
    }

  if (ballBounds.topLeft.axes[1] <= 0){ //Ball reached paddleOne goal, reset ball at top left
    ml->velocity.axes[0] = 3;
    ml->velocity.axes[1] = 3;
    newPos.axes[0] = 30;
    newPos.axes[1] = 30;
    ballLayer.color = COLOR_RED;
    scoreTwo++;
  }else if (ballBounds.botRight.axes[1] >= 159){ //Ball reached paddleTwo goal, reset botright
    ml->velocity.axes[0] = -3;
    ml->velocity.axes[1] = -3;
    newPos.axes[0] = 97;
    newPos.axes[1] = 129;
    ballLayer.color = COLOR_GREEN;
    scoreOne += 1;
  }
  ml->layer->posNext = newPos;
} /**< for ml */
  
  /*
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    
    if ((shapeBoundary.topLeft.axes[0] <= 0) || (shapeBoundary.botRight.axes[0] >= 127)){
      int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
      newPos.axes[0] += (2*velocity);
    }
    if (shapeBoundary.topLeft.axes[1] <= 0){
      ml->velocity.axes[0] = 3;
      ml->velocity.axes[1] = 3;
      newPos.axes[0] = 30;
      newPos.axes[1] = 30;
    }else if (shapeBoundary.botRight.axes[1] >= 159){
      ml->velocity.axes[0] = -3;
      ml->velocity.axes[1] = -3;
      newPos.axes[0] = 97;
      newPos.axes[1] = 129;
    }
    ml->layer->posNext = newPos;
  } /**< for ml */

void movPaddleLeft(Layer *paddleLayer)
{
  Region bounds;
  layerGetBounds(paddleLayer, &bounds);

  if (bounds.topLeft.axes[0] > 0){
    paddleLayer->posNext.axes[0] -= 16;
  }
}

void movPaddleRight(Layer *paddleLayer)
{
  Region bounds;
  layerGetBounds(paddleLayer, &bounds);

  if (bounds.botRight.axes[0] < 127){
    paddleLayer->posNext.axes[0] += 16;
  }
}

void movPaddle(char option)
{
  switch (option){
  case 1:
    movPaddleLeft(&paddleOneLayer);
    break;
  case 2:
    movPaddleRight(&paddleOneLayer);
    break;
  case 3:
    movPaddleLeft(&paddleTwoLayer);
    break;
  case 4:
    movPaddleRight(&paddleTwoLayer);
    break;
  default: break;
  }
}

void getScoreText()
{
  sprintf(scoreText, "%d-%d", scoreOne, scoreTwo);
}

void main()
{

  configureClocks();
  lcd_init();
  p2sw_init(0xf);

  layerInit(&ballLayer);
  layerDraw(&ballLayer);

  enableWDTInterrupts();
  or_sr(0x8);

  buzzer_init();
  buzzer_set_period(0);

  //scoreText = (char *)malloc(6 * sizeof(char));
  //getScoreText();
  drawString8x12(50, 75, scoreText, COLOR_WHITE, COLOR_BLACK);
   
  while(1){
    while(!redrawScreen){
      or_sr(0x10);
    }
    redrawScreen = 0;
    movLayerDraw(&ball, &ballLayer);
    //getScoreText();
    drawString8x12(50, 75, scoreText, COLOR_WHITE, COLOR_BLACK);
  
    for (int row = 1; row < 30; row++){
      for (int col = 1; col < (row+(row-1)); col++) {
	  drawPixel((col+15)+(47-row), row+35, COLOR_GOLD);
      }
    }
	     
  }
}

 /** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  // P1OUT |= GREEN_LED;		    //  < Green LED on when cpu on 
  count ++;
  if (count == 15) {
    ballAdvance(&ball);
    // if (p2sw_read())
    // redrawScreen = 1;
    
    switches = p2sw_read();

    if (switches & 0x0100 && switches & 0x01){
      movPaddle(1);
    }
    if (switches & 0x0200 && switches & 0x02){
      movPaddle(2);
    }
    if (switches & 0x0400 && switches & 0x04){
      movPaddle(3);
    }
    if (switches & 0x0800 && switches & 0x08){
      movPaddle(4);
    } 
    
    count = 0;
    redrawScreen = 1;

    if (playSound){
      buzzer_set_period(2500);
      playSound = 0;
    }else{
      buzzer_set_period(0);
    }

    if (scoreOne > 9 || scoreTwo > 9){
      scoreOne = scoreTwo = 0;
    }
  } 
  P1OUT &= ~GREEN_LED;		   // < Green LED off when cpu off */
  /////////////
}
