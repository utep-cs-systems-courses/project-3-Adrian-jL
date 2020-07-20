/** \file lcddemo.c
 *  \brief A simple demo that draws a string and square
 */

#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"

/** Initializes everything, clears the screen, draws "hello" and a square */
int
main()
{
  configureClocks();
  lcd_init();
  u_char width = screenWidth, height = screenHeight;

  clearScreen(COLOR_NAVY);

  drawString8x12 (10,10,"Howdy", COLOR_WHITE, COLOR_SKY_BLUE);

  //illRectangle(30,30, 60, 60, COLOR_ORANGE);
  /*
  int j;
  for (j = 0; j < 60; j++){
    int row = j;
    for (int col = j; col <= screenWidth + j; col++){
      drawPixel(row, col, COLOR_PINK);
    }
  }
  */
}
