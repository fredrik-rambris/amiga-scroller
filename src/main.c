#include <stdio.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>

#include "mem.h"

UWORD blankPointer[2] = {0x0000, 0x0000};


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

    struct BitMap *bmap;

    bmap = AllocateBitMapManual(640 + 16, 16, 2);

    struct NewScreen ns = {
        0, 200, 640, 16,
        2,
        1, 0,
        HIRES,
        CUSTOMSCREEN | SCREENQUIET | CUSTOMBITMAP,
        NULL,
        "Text Test",
        NULL,
        bmap
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
        1, // Width in words (16 pixels / 16 = 1 word)
        1, // Height in pixels
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

/* #define SCROLL_VPORT */

int main(void) {
    struct Screen *screen;
    struct RastPort *rp;
    UBYTE *scrollText, *cur;
    WORD i;

    CloseWorkBench();

    if ((screen = open_screen())) {
        rp = &screen->RastPort;
        if ((scrollText = ReadFile("scroll.txt"))) {
            for (;;) {
                cur = scrollText;
                while (*cur) {
                    if (*cur == '@') {
                        SetAPen(rp, cur[1] - '0');
                        cur += 2;
                    }
                    if (*cur < ' ') {
                        *cur=' ';
                    }

#ifdef SCROLL_VPORT
                    screen->ViewPort.DxOffset = rp->Font->tf_XSize;
                    ScrollVPort(&screen->ViewPort);
                    ScrollRaster(rp, rp->Font->tf_XSize, 0, 0, 0, 640 + 15, 15);
#endif
                    Move(rp, 640, rp->Font->tf_Baseline);
                    Text(rp, cur, 1);
#ifdef SCROLL_VPORT
                    while (screen->ViewPort.DxOffset--) {
                        ScrollVPort(&screen->ViewPort);
                    }
#else
                    i=rp->Font->tf_XSize;
                    while (i--) {
                        ScrollRaster(rp, 1, 0, 0, 0, 640 + 15, 15);
                        WaitTOF();
                    }
#endif

                    cur++;
                }
            }

            FreeVec(scrollText);
        }
        close_screen(screen);
    }

    OpenWorkBench();

    return 0;
}
