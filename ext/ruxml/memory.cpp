#include <cstring>
#include <cstdarg>
#include <cstdio>

#include "memory.hpp"
#include "str.hpp"

Allocator* temp_allocator = nullptr;

void *allocate_size_(void *data, size_t size) {
   if (data) {
      return ((Allocator *) data)->alloc(data, size);
   } else {
      return raw_allocate_size(size);
   }
}

void allocate_free_(void *data, void *ptr) {
   if (data) {
      ((Allocator *) data)->free(data, ptr);
   } else {
      raw_free(ptr);
   }
}

#ifdef MEMORY_DEBUG2
void *alloc_size_(void *data, size_t size, const char *file, int line) {
   printf("%s:%i ALLOC  : %li\n", file, line, size);
   alloc_size_(data, size);
}

void alloc_free_(void *data, void *ptr, const char *file, int line) {
   printf("%s:%i FREE\n", file, line);
   alloc_free_(data, ptr);
}
#endif

void arena_init(MemoryArena *arena, const char *name, Allocator *allocator) {
#ifdef MEMORY_DEBUG
   arena->name = name;
   printf("Arena init: %s\n", name);
#endif

   arena->allocator.alloc = allocator_arena_alloc;
   arena->allocator.free = allocator_noop_free;
   arena->base_allocator = allocator;
   arena->min_block_size = 8 * 1024 * 1024;
   arena->block_first = nullptr;
   arena->block_last = nullptr;
   arena->free = nullptr;
}

void arena_clear(MemoryArena *arena) {
#ifdef MEMORY_INFO
   arena->memory_allocated = 0;
#endif

   if (arena->block_first) {
      assert(arena->block_last);
      arena->block_last->next = arena->free;
      arena->free = arena->block_first;
      arena->block_first = nullptr;
      arena->block_last = nullptr;
   }
}

void arena_destroy(MemoryArena *arena) {
#ifdef MEMORY_DEBUG
   printf("Arena destroy: %s\n", arena->name);
#endif
   while (arena->block_first) {
      void *mem = arena->block_first;
      arena->block_first = arena->block_first->next;
      allocate_free(arena->base_allocator, mem);
   }

   while (arena->free) {
      void *mem = arena->free;
      arena->free = arena->free->next;
      allocate_free(arena->base_allocator, mem);
   }

   arena->block_last = nullptr;
}

void arena_stats(MemoryArena *arena, uint64_t *allocated_ptr, uint64_t *used_ptr) {
   uint64_t allocated = 0;
   uint64_t used = 0;

   auto block = arena->block_first;
   while (block) {
      allocated += block->size;
      used += block->used;
      block = block->next;
   }

   *allocated_ptr = allocated;
   *used_ptr = used;
}

TempSection begin_temp_section(MemoryArena *arena) {
   TempSection section = {};

   if (!arena->block_first) {
      arena_alloc_block(arena, 0);
   }

   auto block = arena->block_first;
   block->temp_count++;

   section.arena = arena;
   section.block = block;
   section.mark = block->used;
   return section;
}

void end_temp_section(TempSection section) {
   MemoryArena *arena = section.arena;

   while (arena->block_first != section.block) {
      assert(arena->block_first);
      assert(arena->block_first->temp_count == 1);
      void *mem = arena->block_first;
      arena->block_first = arena->block_first->next;
      free(mem);
   }
   assert(arena->block_first == section.block);

   arena->block_first->used = section.mark;
   arena->block_first->temp_count--;
}

void arena_alloc_block(MemoryArena *arena, size_t size) {
   auto cur_block = arena->free;
   MemoryArenaBlock *prev_block = nullptr;
   while (cur_block) {
      if (cur_block->size >= size) {
         if (arena->free == cur_block) arena->free = cur_block->next;
         if (prev_block) prev_block->next = cur_block->next;
         cur_block->next = arena->block_first;
         cur_block->used = 0;
         if (arena->block_first) {
            cur_block->temp_count = arena->block_first->temp_count;
         } else {
            arena->block_last = cur_block;
         }

         arena->block_first = cur_block;
         return;
      }
      prev_block = cur_block;
      cur_block = cur_block->next;
   }

   size_t required_size = arena->min_block_size > size ? arena->min_block_size : size;

   size_t size_with_header = sizeof(MemoryArenaBlock) + required_size;
   auto mem = (uint8_t *) raw_allocate_size_zero(size_with_header);
   auto header = (MemoryArenaBlock *) mem;
   header->data = mem + sizeof(MemoryArenaBlock);
   header->size = required_size;

   if (arena->block_first) {
      header->temp_count = arena->block_first->temp_count;
   }

   header->next = arena->block_first;
   arena->block_first = header;
   if (!arena->block_last) arena->block_last = arena->block_first;
}

void *arena_alloc(MemoryArena *arena, size_t size) {
#ifdef MEMORY_INFO
   arena->memory_allocated += size;
#endif

   if (!arena->block_first || (arena->block_first->used + size > arena->block_first->size)) {
      arena_alloc_block(arena, size);
   }
   assert(arena->block_first);
   assert(arena->block_last);
   assert(arena->block_first->used + size <= arena->block_first->size);

   auto mem = arena->block_first->data + arena->block_first->used;
   arena->block_first->used += size;

   return mem;
}

char *zstr_dup(Allocator *allocator, const char *str, int64_t size) {
   auto *buffer = (char *) allocate_size(allocator, size + 1);
   memcpy(buffer, str, sizeof(char) * size);
   return buffer;
}

char *zstr_dup(Allocator *allocator, const char *str) {
   return zstr_dup(allocator, str, zstr_length(str));
}

char *zstr_print(Allocator *allocator, const char *fmt, ...) {
   char buffer[1024];
   const size_t buffer_size = array_size(buffer);

   va_list v;
   va_start(v, fmt);
   auto res = vsnprintf(buffer, buffer_size, fmt, v);
   va_end(v);
   assert(res >= 0);
   if (res < buffer_size) {
      buffer[res] = 0;
      return zstr_dup(allocator, buffer, res + 1);
   }

   auto long_size = static_cast<size_t>(res + 1);
   auto long_buffer = (char *) allocate_size(allocator, long_size);
   va_start(v, fmt);
   vsnprintf(long_buffer, long_size, fmt, v);
   va_end(v);

   long_buffer[res] = 0;
   return long_buffer;
}