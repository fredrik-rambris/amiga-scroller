#ifndef GRAPHICS_H
#define GRAPHICS_H

struct Scroller {
  struct RastPort* rp;
  struct RastPort* dest_rp;
  UWORD width;
  UWORD height;
  UWORD dest_x;
  UWORD dest_y;
  STRPTR text;
  ULONG text_pos;
  UWORD rect_pos;
  UWORD rect_pos_max;
  UWORD fill_column_pos;
};

BOOL initScroller(struct Scroller* scroller, UWORD width, UWORD dest_x, UWORD dest_y, struct RastPort* dest_rp, STRPTR text, struct TextFont* font);
VOID freeScroller(struct Scroller* scroller);
VOID scroll(struct Scroller* scroller);

/**************************************
 * Rastport flags for allocRastPort() *
 **************************************/
#define RPF_LAYER   0x01
#define RPF_BITMAP  0x02
#define RPF_TMPRAS  0x04
#define RPF_AREA    0x08
#define RPF_ALL     0x0F

struct RastPort* allocRastPort(ULONG sizex, ULONG sizey, ULONG depth, ULONG bm_flags, struct BitMap *friend, ULONG rp_flags, LONG max_vectors);
VOID freeRastPort(struct RastPort* rp, ULONG free_flags);

#endif //GRAPHICS_H
