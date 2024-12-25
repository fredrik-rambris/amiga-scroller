
#include <clib/exec_protos.h>


APTR myallocvec(ULONG byteSize, ULONG requirements) {
    APTR mem;
    byteSize += sizeof(APTR);
    mem = AllocMem(byteSize, requirements);
    *(ULONG *) mem = byteSize;
    mem += sizeof(APTR);
    return mem;
}

void myfreevec(APTR memory) {
    if (!memory) {
        return;
    }
    ULONG byteSize;
    memory -= sizeof(APTR);
    byteSize = *(ULONG *) memory;
    FreeMem(memory, byteSize);
}
