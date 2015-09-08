/* Purpose: Basic Scrolling, font is fixed!
 * Logic:   Maintain a scroll buffer and also overrwrite the
 *          oldest entry. Redraw the buffer if there is a change
 *          in scroll buffer.
 * Author:  Vigith Maurice <v@vigith.com>
 */

#include <U8glib.h>
#include <stdlib.h>
#include <string.h>

#define SCROLL_LEN 6            /* height w.r.t OLED font */
#define SCROLL_WIDTH 16         /* width w.r.t OLED font */

int scroll_postn = 0;           /* current scroll postn, pointer to next element to be overwritten (scroll_postn - 1 is the latest) */
char *scroll[8];                /* scroll buffer */
boolean update_scroll = false;

char int_to_ascii[4];           /* itoa */

// U8GLIB_DOGM128(sck, mosi(sd1), cs, a0(d/c) [, reset])
U8GLIB_SSD1306_64X48 u8g(4, 5, 6, 7, 8);

/* stores only the latest X elements in scroll buffer */
void update_scroll_buffer(char *str) {
  /* copy the str to current scroll_postn */
  strcpy(scroll[scroll_postn], str);
  /* make scroll buffer like a circular buffer */
  scroll_postn = (scroll_postn + 1) % (SCROLL_LEN);

  /* we have made an update */
  update_scroll = true;
  
  return;
}

/* a small counter that just counts! */
void counter() {  
  static short int i = 0;
  if (i > 15) {
    return;
  }
  /* for effect, else it is too fast for eye :-) */
  delay(1000);
  /* convert to ascii */
  itoa (i, int_to_ascii, 10);
  i++;
  /* update the scroll */
  update_scroll_buffer(int_to_ascii);
  return;
}

void draw(){
  u8g.setFontRefHeightExtendedText();
  /* get the height of the font */
  int h = u8g.getFontAscent()-u8g.getFontDescent();    

  /* lets drawn such that the latest entry will be on the last line */
  int current = scroll_postn;   /* scroll_postn points to the oldest element (one that is going to get overwritten */
  short int line = 1;           /* line number */
  do {
    /* print the oldest */
    u8g.drawStr(0, h * line, scroll[current]);
    /* move to next one, reset to mimic circular buffer */
    current = (current + 1) % SCROLL_LEN;
    /* last line is the lastest */
    line++;
  } while (current != scroll_postn); /* we started with current = scroll_postn */

  /* reset update_scroll to false, we just drew that latest scroll buffer */
  update_scroll = false;

  return;  
}


void setup() {  
  u8g.setFont(u8g_font_04b_03r);
  u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.

  /* initiallize the array */
  for (int i=0; i< SCROLL_LEN; i++) {
    /* there is no call to free, we can't call! */
    scroll[i] = (char *)malloc(sizeof(char) *SCROLL_WIDTH);
    /* oops, can't find bzero */
    memset(scroll[i], 0, SCROLL_WIDTH);
  }

  return;
}

void loop() {  
  u8g.firstPage();
  /* if scroll buffer has changed, then draw */
  if (update_scroll) {
    do {  
      draw();
    } while( u8g.nextPage() );
  }
  
  /* update the counter */
  counter();
}
  
