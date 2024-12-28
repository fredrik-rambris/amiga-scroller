#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/display.h>
#include <graphics/layers.h>
#include <graphics/clip.h>

#include <clib/exec_protos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>

#include "graphics.h"

//initScroller(&scroller, width, dest_x, dest_y, dest_rp, text, font)
/******************************************************************************
 * Initializes a struct Scroller to initial values and does the necessary     *
 * allocations.                                                               *
 * INPUTS                                                                     *
 *     scroller - address of the scroller to initialze                        *
 *     width    - width of the scroller strip                                 *
 *     dest_x   - x coordinate on dest_rp to display the scroller             *
 *     dest_y   - x coordinate on dest_rp to display the scroller             *
 *     text     - text to scroll                                              *
 *     font     - A fixed width Amiga font to display the scroll text         *
  *****************************************************************************/
BOOL initScroller(struct Scroller* scroller, UWORD width, UWORD dest_x, UWORD dest_y, struct RastPort* dest_rp, STRPTR text, struct TextFont* font)
{
  UWORD height = 0;
  UWORD font_width = 0;

  if (font) {
    font_width = font->tf_XSize;
    height = font->tf_YSize;
  }
  else {
    font_width = dest_rp->Font->tf_XSize;
    height = dest_rp->Font->tf_YSize;
  }

  //Scroller width must be a multiple of font width
  width = width - (width % font_width);

  scroller->rp = allocRastPort((width + font_width) * 2, height, dest_rp->BitMap->Depth, dest_rp->BitMap->Flags | BMF_CLEAR, dest_rp->BitMap, RPF_BITMAP, 0);
  if (scroller->rp) {
    if (font) SetFont(scroller->rp, font);
    scroller->dest_rp = dest_rp;
    scroller->width = width;
    scroller->height = height;
    scroller->dest_x = dest_x;
    scroller->dest_y = dest_y;
    scroller->text = text;
    scroller->text_pos = 0;
    scroller->rect_pos = font_width;
    scroller->rect_pos_max = width + font_width + font_width;
    scroller->fill_column_pos = 0;

    return TRUE;
  }

  return FALSE;
}

//freeScroller(&scroller)
/******************************************************************************
 * Frees the allocations made on a struct Scroll by initScroller()            *
 ******************************************************************************/
VOID freeScroller(struct Scroller* scroller)
{
  if (scroller) freeRastPort(scroller->rp, RPF_BITMAP);
}

//scroll(&scroller)
/******************************************************************************
 * Scrolls a scroller 1 px and does the necessary Text() and blit operations. *
 ******************************************************************************/
VOID scroll(struct Scroller* scroller)
{
  //if blit rectangle has reached the edge of the bitmap
  if (scroller->rect_pos >= scroller->rect_pos_max) {
    scroller->fill_column_pos = 0;
    scroller->rect_pos = scroller->rp->Font->tf_XSize;
  }
  //if the scroller has scrolled one whole letter from the text
  if (scroller->rect_pos % scroller->rp->Font->tf_XSize == 0) {
    static STRPTR letter = " ";
    *letter = scroller->text[scroller->text_pos];

    //if the scroller reached the end of text
    if (!*letter) {
      scroller->text_pos = 0;
      *letter = scroller->text[0];
    }
    //Character @ defines a text color change
    if (*letter == '@') {
      SetAPen(scroller->rp, scroller->text[scroller->text_pos + 1] - '0');
      scroller->text_pos += 2;
      *letter = scroller->text[scroller->text_pos];
    }
    //New lines and all other white space is regarded as a single space character
    if (*letter < ' ') *letter = ' ';

    //Text() the left fill column with the incoming letter
    Move(scroller->rp, scroller->fill_column_pos, scroller->rp->Font->tf_Baseline);
    Text(scroller->rp, letter, 1);
    //Text() the right fill column with the incoming letter
    Move(scroller->rp, scroller->fill_column_pos + scroller->width + scroller->rp->Font->tf_XSize, scroller->rp->Font->tf_Baseline);
    Text(scroller->rp, letter, 1);

    scroller->text_pos++;
    scroller->fill_column_pos += scroller->rp->Font->tf_XSize;
  }

  BltBitMapRastPort(scroller->rp->BitMap, scroller->rect_pos, 0, scroller->dest_rp, scroller->dest_x, scroller->dest_y, scroller->width, scroller->height, (ABC|ABNC));
  scroller->rect_pos++;
}

//allocRastPort(sizex, sizey, depth, bm_flags, friend, rp_flags, max_vectors)
/******************************************************************************
 * Allocates a BitMap alongside with a RastPort which is required to do draws *
 * on it. It can also create and initialize the Layer, TmpRas and AreaInfo    *
 * structs alogside with it.                                                  *
 * A function which should have been implemented in the API I guess.          *
  *****************************************************************************/
struct RastPort* allocRastPort(ULONG sizex, ULONG sizey, ULONG depth, ULONG bm_flags, struct BitMap *friend, ULONG rp_flags, LONG max_vectors)
{
  struct RastPort* rp = AllocMem(sizeof(struct RastPort), MEMF_ANY);
  if (rp) {
    InitRastPort(rp);
    if (rp_flags & RPF_BITMAP) {
      rp->BitMap = AllocBitMap(sizex, sizey, depth, bm_flags, friend);
      if (rp->BitMap) {
        if (rp_flags & RPF_LAYER) {
          struct Layer_Info* li = NewLayerInfo();
          if (li) {
            rp->Layer = CreateUpfrontLayer(li, rp->BitMap, 0, 0, sizex, sizey, LAYERSIMPLE, NULL);
          }
        }
      }
    }
    if (rp_flags & RPF_TMPRAS) {
      rp->TmpRas = AllocMem(sizeof(struct TmpRas), MEMF_ANY);
      if (rp->TmpRas) {
        UBYTE* tmpRasBuff = AllocMem(RASSIZE(sizex, sizey), MEMF_CHIP | MEMF_CLEAR);
        if (tmpRasBuff) {
          InitTmpRas(rp->TmpRas, tmpRasBuff, RASSIZE(sizex, sizey));
        }
        else {
          FreeMem(rp->TmpRas, sizeof(struct TmpRas));
          rp->TmpRas = NULL;
        }
      }
    }
    if (rp_flags & RPF_AREA) {
      rp->AreaInfo = AllocMem(sizeof(struct AreaInfo), MEMF_ANY);
      if (rp->AreaInfo) {
        UBYTE* areaInfoBuff = AllocMem(max_vectors * 5, MEMF_ANY | MEMF_CLEAR);
        if (areaInfoBuff) {
          InitArea(rp->AreaInfo, areaInfoBuff, max_vectors);
        }
        else {
          FreeMem(rp->AreaInfo, sizeof(struct AreaInfo));
          rp->AreaInfo = NULL;
        }
      }
    }
  }

  return rp;
}

//freeRastPort(rastPort)
/******************************************************************************
 * Only to free RastPorts created by allocRastPort().                         *
 * free_flags determine which components to be freed along with the rastport. *
 * ie. you can create a rastport with RPF_ALL, create graphics on its BitMap  *
 * using rasport funtions, then free it with (RPF_ALL & ~RPF_BITMAP) so that  *
 * the bitmap allocated remains but the Layer, TmpRas and AreaInfo are freed. *
 * WARNING: Do not forget to take pointers to components excluded from        *
 * freeing beforehand. Otherwise you'll have no way to free them and so a     *
 * memory leak!                                                               *
 ******************************************************************************/
VOID freeRastPort(struct RastPort* rp, ULONG free_flags)
{
  if (rp) {
    if (free_flags & RPF_AREA && rp->AreaInfo) {
      FreeMem(rp->AreaInfo->VctrTbl, rp->AreaInfo->MaxCount * 5);
      FreeMem(rp->AreaInfo, sizeof(struct AreaInfo));
    }
    if (free_flags & RPF_TMPRAS && rp->TmpRas) {
      FreeMem(rp->TmpRas->RasPtr, rp->TmpRas->Size);
      FreeMem(rp->TmpRas, sizeof(struct TmpRas));
    }
    if (free_flags & RPF_LAYER && rp->Layer) {
      struct Layer_Info* li = rp->Layer->LayerInfo;
      DeleteLayer(0L, rp->Layer);
      DisposeLayerInfo(li);
    }
    if (free_flags & RPF_BITMAP && rp->BitMap) FreeBitMap(rp->BitMap);

    FreeMem(rp, sizeof(struct RastPort));
  }
}
