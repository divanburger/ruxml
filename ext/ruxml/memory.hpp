#pragma once

#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <cstdio>

#define array_size(_arr) ((int)(sizeof(_arr)/sizeof(*(_arr))))

#define raw_allocate_size(size) (malloc(size))
#define raw_allocate_size_zero(size) (calloc(1, size))
#define raw_allocate_type(type) ((type *) malloc(sizeof(type)))
#define raw_allocate_type_zero(type) ((type *) calloc(1, sizeof(type)))
#define raw_allocate_array(type, size) ((type *) malloc(sizeof(type) * size))
#define raw_allocate_string(size) ((char *) malloc(sizeof(char)*size))
#define raw_allocate_string_zt(size) ((char *) calloc(1, sizeof(char)*(size+1)))
#define raw_free(ptr) (free(ptr))

/// ALLOCATOR

using AllocatorAllocFunc = void *(*)(void *data, size_t size);
using AllocatorFreeFunc = void (*)(void *data, void *ptr);

struct Allocator {
   AllocatorAllocFunc alloc;
   AllocatorFreeFunc free;
};

inline void *allocator_raw_alloc(void *data, size_t size) {
   return raw_allocate_size(size);
}

inline void allocator_raw_free(void *data, void *ptr) {
   raw_free(ptr);
}

inline void allocator_noop_free(void *data, void *ptr) {
}

static Allocator raw_allocator = {allocator_raw_alloc, allocator_raw_free};

inline Allocator *make_raw_allocator() { return &raw_allocator; }

void *allocate_size_(void *data, size_t size);
void allocate_free_(void *data, void *ptr);

#define allocate_size(allocator, size) (allocate_size_(allocator, size))
#define allocate_type(allocator, type) ((type*)allocate_size_(allocator, sizeof(type)))
#define allocate_array(allocator, type, count) ((type*)allocate_size_(allocator, sizeof(type) * (count)))
#define allocate_string(allocator, size) ((char *)allocate_size_(allocator, size))
#define allocate_zstring(allocator, size) ((char *)allocate_size_(allocator, size + 1))
#define allocate_free(allocator, ptr) (allocate_free_(allocator, ptr))

/// ARENA

struct MemoryArenaBlock {
   size_t used;
   size_t size;
   MemoryArenaBlock *next;

   uint8_t temp_count;
   uint8_t *data;
};

struct MemoryArena {
   Allocator allocator;
   Allocator *base_allocator;
   size_t min_block_size;
   MemoryArenaBlock *block_first;
   MemoryArenaBlock *block_last;
   MemoryArenaBlock *free;

#ifdef MEMORY_INFO
   size_t memory_allocated;
#endif

#ifdef MEMORY_DEBUG
   const char *name;
#endif
};

struct TempSection {
   MemoryArena *arena;
   MemoryArenaBlock *block;
   size_t mark;
};

void arena_init(MemoryArena *arena, const char *name, Allocator *allocator = make_raw_allocator());
void arena_clear(MemoryArena *arena);
void arena_destroy(MemoryArena *arena);
void arena_stats(MemoryArena *arena, uint64_t *allocated_ptr, uint64_t *used_ptr);
void arena_alloc_block(MemoryArena *arena, size_t size);
void *arena_alloc(MemoryArena *arena, size_t size);

#define arena_alloc_type(allocator, type) ((type*)arena_alloc(allocator, sizeof(type)))

inline Allocator *arena_allocator(MemoryArena *arena) { return (Allocator *) (void *) arena; }

TempSection begin_temp_section(MemoryArena *arena);
void end_temp_section(TempSection section);

inline void *allocator_arena_alloc(void *data, size_t size) {
   return arena_alloc((MemoryArena *) data, size);
}

extern Allocator* temp_allocator;

/// STRING

char *zstr_dup(Allocator *allocator, const char *str, int64_t size);
char *zstr_dup(Allocator *allocator, const char *str);
char *zstr_print(Allocator *allocator, const char *fmt, ...);