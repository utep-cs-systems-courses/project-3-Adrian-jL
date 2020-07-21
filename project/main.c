
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#define GREEN_LED BIT6


typedef struct MovLayer_s{
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

//TODO: change starting point
AbRect paddleOne = {abRectGetBounds, abRectCheck, {8, 2}};
AbRect paddleTwo = {abRectGetBounds, abRectCheck, {8, 2}};

Layer paddleOneLayer = {(AbShape*)&paddleOne, {6, 5}, {0, 0}, {0, 0}, COLOR_WHITE, 0};
Layer paddleTwoLayer = {(AbShape*)&paddleTwo, {120, 155}, {0, 0}, {0, 0}, COLOR_WHITE,
                        &paddleOneLayer};

Layer ballLayer = {(AbShape*)&circle6, {30, 30}, {0, 0}, {0, 0}, COLOR_RED, &paddleTwoLayer};
MovLayer ball = {&ballLayer, {1, 1}, 0};


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

//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    /*   for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }	/**< if outside of fence 
    } /**< for axis */
    if ((shapeBoundary.topLeft.axes[0] <= 0) || (shapeBoundary.botRight.axes[0] >= 127)){
      int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
      newPos.axes[0] += (2*velocity);
    }
    if ((shapeBoundary.topLeft.axes[1] <= 0) || (shapeBoundary.botRight.axes[1] >= 159)){
      int velocity = ml->velocity.axes[0] = 1;
      ml->velocity.axes[1] = -1;
      newPos.axes[0] = 30;
      newPos.axes[1] = 30;
    }
    ml->layer->posNext = newPos;
  } /**< for ml */
}

void movPaddleLeft(Layer *paddleLayer)
{
  Region bounds;
  layerGetBounds(paddleLayer, &bounds);

  if (bounds.topLeft.axes[0] > 0){
    paddleLayer->pos.axes[0] -= 8;
  }
}

void movPaddleRight(Layer *paddleLayer)
{
  Region bounds;
  layerGetBounds(paddleLayer, &bounds);

  if (bounds.botRight.axes[0] < 127){
    paddleLayer->pos.axes[0] += 8;
  }
}

u_int switches;
char dir = 1;
u_int bgColor = COLOR_BLACK;
int redrawScreen = 1;

void main()
{

  configureClocks();
  lcd_init();
  p2sw_init(0x15);

  layerInit(&ballLayer);
  layerDraw(&ballLayer);

  enableWDTInterrupts();
  or_sr(0x8);

  while(1){
    while(!redrawScreen){
  
      or_sr(0x10);
    }
    redrawScreen = 0;
    movLayerDraw(&ball, &ballLayer);
  }
}

 /** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  // P1OUT |= GREEN_LED;		    //  < Green LED on when cpu on 
  count ++;
  if (count == 15) {
    mlAdvance(&ball);
    //
    /*  Region bounds;
    layerGetBounds(&paddleOneLayer, &bounds);

    if (bounds.topLeft.axes[0] <= 0){
      dir = 1;
    }
     else if (bounds.botRight.axes[0] >= 127){
      dir = 0;
     }

    if (dir){
      movPaddleRight(&paddleOneLayer);
    }else{
      movPaddleLeft(&paddleOneLayer);
    */
    switches = p2sw_read();
    // if (switches & 0xf0)
    // redrawScreen = 1;
    count = 0;
  } 
  // P1OUT &= ~GREEN_LED;		   // < Green LED off when cpu off */
  /////////////
}
