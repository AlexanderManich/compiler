#include "compiler.h"

typedef struct Arena Arena;
struct Arena {
    u64 pos;
    u8 *base;
    u64 size;
};

// create or destroy a 'stack' - an "arena"
Arena *ArenaAlloc(void);
void ArenaRelease(Arena *arena);

// push some bytes onto the 'stack' - the way to allocate
void *ArenaPush(Arena *arena, u64 size);
void *ArenaPushZero(Arena *arena, u64 size);

// some macro helpers that I've found nice:
#define PushArray(arena, type, count) (type *)ArenaPush((arena), sizeof(type)*(count))
#define PushArrayZero(arena, type, count) (type *)ArenaPushZero((arena), sizeof(type)*(count))
#define PushStruct(arena, type) PushArray((arena), (type), 1)
#define PushStructZero(arena, type) PushArrayZero((arena), (type), 1)

// pop some bytes off the 'stack' - the way to free
void ArenaPop(Arena *arena, u64 size);

// get the # of bytes currently allocated.
u64 ArenaGetPos(Arena *arena);

// also some useful popping helpers:
void ArenaSetPosBack(Arena *arena, u64 pos);
void ArenaClear(Arena *arena);