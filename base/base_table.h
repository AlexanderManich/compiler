#ifndef BASE_TABLE_H
#define BASE_TABLE_H

#include "base_include.h"

typedef u32 (*HashFunction)(void *);
typedef b32 (*CompareFunction)(void *, void *);
typedef void *(*CopyFunction)(Arena *, void *);


typedef struct TableEntry {
    u32 hash;
    void *key;
    void *value;
} TableEntry;

typedef struct Table {
    u32 count;
    u32 capacity;

    Arena *arena;

    TableEntry *entries;
    // NOTE: Table implementation assumes that we can't hash to 0!
    u32 (*hash_function)(void *);
    b32 (*compare_function)(void *, void *);
    void *(*copy_function)(Arena *, void *);
} Table;

static void table_init(Table *table, Arena *arena, u32 capacity, HashFunction hash_function, CompareFunction compare_function, CopyFunction copy_function);

static void table_rehash(Table *table, TableEntry *old_entries, u32 old_capacity);

static void table_expand(Table *table);

static void *table_get_(Table *table, void *key);

static void *table_add(Table *table, void *key, void *value);

static void *table_intern(Table *table, void *key);

static void *table_str8_copy(Arena *arena, void *key);
static u32 table_str8_hash(void *key);
static b32 table_str8_compare(void *s1, void *s2);



#endif // BASE_TABLE_H