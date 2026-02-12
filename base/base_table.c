#include "base_table.h"

static void table_init(Table *table, Arena *arena, u32 capacity, HashFunction hash_function, CompareFunction compare_function, CopyFunction copy_function) {
    debug_assert(is_pow2(capacity));
    table->capacity = capacity;
    table->count = 0;
    table->arena = arena;
    table->entries = push_array(arena, TableEntry, table->capacity);
    table->hash_function = hash_function;
    table->compare_function = compare_function;
    table->copy_function = copy_function;
}

static void table_rehash(Table *table, TableEntry *old_entries, u32 old_capacity) {
    for (u32 i = 0; i < old_capacity; i++) {
        TableEntry entry = old_entries[i];
        if (!entry.hash) continue;

        u32 index = entry.hash & (table->capacity - 1);

        for (;;) {
            TableEntry *slot = &table->entries[index];
            if (!slot->hash) {
                *slot = entry; 
                break;
            } 
            index = (index + 1) & (table->capacity - 1);
        }
    }
}

static void table_expand(Table *table) {
    u32 old_capacity = table->capacity; 
    TableEntry *old_entries = table->entries;

    table->capacity *= 2;
    // TODO: We need to free the old entries 
    table->entries = push_array(table->arena, TableEntry, table->capacity);

    // NOTE: Maybe we should just table_add() all the old entries?
    if (table->count) {
        table_rehash(table, old_entries, old_capacity);
    }
}

static void *table_get_(Table *table, void *key) {
    u32 hash = table->hash_function(key);
    u32 index = hash & (table->capacity - 1);

    for (;;) {
        TableEntry *entry = &table->entries[index];
        if (!entry->hash) return 0;
        if (table->compare_function(entry->key, key)) return entry->value;
        
        index = (index + 1) & (table->capacity - 1);
    }

    return 0;
}

static void *table_add(Table *table, void *key, void *value) {
    // TODO: If we at some point want to be able to remove items from the table, we should
    // have a notion of number of valid items in the table AND a count of "slots occupied" e.g. valid items
    // + gravestones 
    if ((table->count + 1) * 10 >= table->capacity * 7) table_expand(table);

    u32 hash = table->hash_function(key);
    u32 index = hash & (table->capacity - 1);

    while (table->entries[index].hash) {
        index = (index + 1) & (table->capacity - 1);
    }

    table->count += 1;
    
    TableEntry *entry = &table->entries[index];
    entry->hash = hash;
    entry->key = key;
    entry->value = value;

    return entry->value;
}

static void *table_intern(Table *table, void *key) {
    void *found = table_get_(table, key);
    if (found) return found;

    // Key and underlying buffers need to be stored in the table arena
    void *canonical = table->copy_function(table->arena, key);
    table_add(table, canonical, canonical);

    return canonical;
}

static void *table_str8_copy(Arena *arena, void *key) {
    String8 string = *(String8 *)key;
    String8 *result = push_array(arena, String8, 1);
    *result = str8_copy(arena, string);
    return result;
}

static u32 table_str8_hash(void *key) {
    return str8_hash(*(String8 *)key);
}

static b32 table_str8_compare(void *s1, void *s2) {
    return str8_compare(*(String8 *)s1, *(String8 *)s2);
}