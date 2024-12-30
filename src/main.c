#include <stdio.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>

#include "mem.h"

UWORD __chip blankPointer[2] = {0x0000, 0x0000};


struct BitMap *AllocateBitMapManual(ULONG width, ULONG height, UBYTE depth) {
    struct BitMap *bitmap;
    ULONG bytesPerRow, totalBitplaneSize, totalSize;
    UBYTE i;
    UBYTE *memoryBlock;
    UBYTE *bitplaneMemory;

    // Calculate bytes per row and total bitplane size
    bytesPerRow = RASSIZE(width, 1); // Bytes per row, longword-aligned
    totalBitplaneSize = RASSIZE(width, height); // Size of one bitplane
    totalSize = sizeof(struct BitMap) + (depth * totalBitplaneSize);
    totalSize = (totalSize + 3) & ~3; // Round up to nearest longword

    // Allocate a single block for the struct BitMap and planes
    memoryBlock = AllocVec(totalSize, MEMF_CHIP | MEMF_CLEAR);
    if (!memoryBlock) {
        return NULL; // Allocation failed
    }

    // Point the BitMap to the start of the block
    bitmap = (struct BitMap *) memoryBlock;

    // Initialize the BitMap structure
    bitmap->BytesPerRow = bytesPerRow;
    bitmap->Rows = height;
    bitmap->Depth = depth;

    // Set up pointers for each bitplane within the allocated block
    bitplaneMemory = memoryBlock + sizeof(struct BitMap);
    for (i = 0; i < depth; i++) {
        bitmap->Planes[i] = bitplaneMemory + (i * totalBitplaneSize);
    }

    InitBitMap(bitmap, depth, width, height);

    return bitmap;
}


struct Screen *open_screen() {
    UWORD palette[] = {
        0x000,
        0xccc,
        0xc55,
        0x469
    };

    struct NewScreen ns = {
        0, 200, 640, 16,
        2,
        1, 0,
        HIRES,
        CUSTOMSCREEN | SCREENQUIET,
        NULL,
        "Text Test",
        NULL,
        NULL
    };

    struct NewWindow nw = {
        0, 0, ns.Width, ns.Height, ns.DetailPen, ns.BlockPen,
        0,
        BORDERLESS | BACKDROP | ACTIVATE | RMBTRAP,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        ns.Width, ns.Height,
        ns.Width, ns.Height,
        CUSTOMSCREEN
    };
    struct Screen *screen;
    struct Window *window;

    if (!(screen = OpenScreen(&ns))) return NULL;
    nw.Screen = screen;
    LoadRGB4(&screen->ViewPort, palette, 4);
    SetAPen(&screen->RastPort, ns.DetailPen);

    if (!(window = OpenWindow(&nw))) goto error;

    SetPointer(
        window, // Correct: Pass the pointer to the window
        blankPointer, // Pointer to the blank image data
        0, // Width in words (16 pixels / 16 = 1 word)
        0, // Height in pixels
        0, // X-offset (hotspot)
        0 // Y-offset (hotspot)
    );

    return screen;
error:
    if (window) CloseWindow(window);
    if (screen) CloseScreen(screen);
    return NULL;
}

void close_screen(struct Screen *screen) {
    if (screen) {
        if (screen->FirstWindow) {
            CloseWindow(screen->FirstWindow);
        }
        CloseScreen(screen);
    }
}

static ULONG FileSize(STRPTR filename) {
    BPTR lock = 0;
    ULONG size = 0;
    struct FileInfoBlock fib;

    if ((lock = Lock(filename, ACCESS_READ))) {
        if (Examine(lock, &fib)) {
            size = fib.fib_Size;
        }

        UnLock(lock);
    }

    return size;
}

static UBYTE *ReadFile(STRPTR filename) {
    BPTR file;
    ULONG size;
    UBYTE *buf = NULL;

    if ((size = FileSize(filename))) {
        if ((file = Open(filename, MODE_OLDFILE))) {
            if ((buf = AllocVec(size + 1, MEMF_PUBLIC | MEMF_CLEAR))) {
                if (Read(file, buf, size) == size) {
                    Close(file);
                    return buf;
                }
                FreeVec(buf);
            }
            Close(file);
        }
    }

    return NULL;
}

struct RastPort *create_rastport(struct RastPort *srcRp, ULONG width, ULONG height, UBYTE depth) {
    struct RastPort *rp;

    if ((rp = AllocVec(sizeof(struct RastPort), MEMF_PUBLIC|MEMF_CLEAR))) {
        InitRastPort(rp);
        if ((rp->BitMap = AllocateBitMapManual((width + srcRp->Font->tf_XSize) * 2, height, depth))) {
            return rp;
        }

        rp->BgPen = srcRp->BgPen;
        rp->FgPen = srcRp->FgPen;
        rp->Font = srcRp->Font;


        FreeVec(rp);
    }

    return NULL;
}

void free_rastport(struct RastPort *rp) {
    if (rp) {
        if (rp->BitMap) {
            FreeVec(rp->BitMap);
        }
        FreeVec(rp);
    }
}

int main(void) {
    struct Screen *screen;
    struct RastPort *rp;
    UBYTE *scrollText, *cur;
    WORD i, xsize;
    int tpos = 0, xpos = 0;

    CloseWorkBench();

    if ((screen = open_screen())) {
        if ((rp = create_rastport(&screen->RastPort, screen->Width, screen->Height, screen->BitMap.Depth))) {
            xsize = screen->Width + screen->RastPort.Font->tf_XSize;

            if ((scrollText = ReadFile("scroll.txt"))) {
                for (;;) {
                    cur = scrollText;
                    xpos = rp->Font->tf_XSize;
                    while (*cur) {
                        if (*cur == '@') {
                            SetAPen(rp, cur[1] - '0');
                            cur += 2;
                        }
                        if (*cur < ' ') {
                            *cur = ' ';
                        }

                        Move(rp, tpos, rp->Font->tf_Baseline);
                        Text(rp, cur, 1);
                        Move(rp, tpos + xsize, rp->Font->tf_Baseline);
                        Text(rp, cur, 1);
                        tpos += rp->Font->tf_XSize;

                        i = rp->Font->tf_XSize;
                        while (i--) {
                            WaitTOF();
                            BltBitMapRastPort(rp->BitMap, xpos, 0, &screen->RastPort, 0, 0, screen->Width, rp->Font->tf_YSize, ABC | ABNC);
                            xpos++;
                        }

                        if (xpos > xsize) {
                            xpos = rp->Font->tf_XSize;
                        }
                        if (tpos > screen->Width) {
                            tpos = 0;
                        }

                        cur++;
                    }
                }

                FreeVec(scrollText);
            }
            free_rastport(rp);
        }
        close_screen(screen);
    }

    OpenWorkBench();

    return 0;
}
