
#ifndef MEM_H
#define MEM_H
#include <exec/types.h>
#include <exec/memory.h>

APTR myallocvec(ULONG ByteSize, ULONG Requirements);
void myfreevec(APTR memory);

#ifndef AllocVec
#define AllocVec(ByteSize, Requirements) myallocvec(ByteSize, Requirements)
#endif

#ifndef FreeVec
#define FreeVec(memory) myfreevec(memory)
#endif

#endif //MEM_H
