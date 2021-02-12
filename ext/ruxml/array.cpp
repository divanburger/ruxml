#include <cassert>
#include <cstring>

#include "array.hpp"

void aclear(void* array) {
   if (array) ahdr((array))->len = 0;
}

void *ainit_(void *array, Allocator *allocator) {
   if (array) {
      ahdr(array)->allocator = allocator;
      return array;
   } else {
      auto hdr = allocate_type(allocator, ArrayHeader);
      hdr->allocator = allocator;
      hdr->len = 0;
      hdr->cap = 0;
      return &hdr->buffer;
   }
}

void afree_(void *array) {
   if (array) allocate_free(ahdr(array)->allocator, ahdr(array));
}

void *asetcap_(void *array, uint32_t new_cap, uint32_t element_size) {
   auto hdr = array ? ahdr(array) : nullptr;

   uint32_t new_len = hdr ? (hdr->len < new_cap ? hdr->len : new_cap) : 0;
   uint32_t mem_size = sizeof(ArrayHeader) + new_cap * element_size;

   if (array) {
      auto old_hdr = hdr;
      hdr = (ArrayHeader *) allocate_size(old_hdr->allocator, mem_size);
      hdr->allocator = old_hdr->allocator;
      hdr->len = new_len;
      hdr->cap = new_cap;
      memcpy(&hdr->buffer, array, new_len * element_size);
      allocate_free(hdr->allocator, old_hdr);
   } else {
      hdr = (ArrayHeader *) allocate_size(nullptr, sizeof(ArrayHeader) + new_cap * element_size);
      hdr->allocator = nullptr;
      hdr->len = new_len;
      hdr->cap = new_cap;
   }

   return &hdr->buffer;
}

void *afit_(void *array, uint32_t count, uint32_t element_size) {
   uint32_t capacity = acap(array);
   if (count <= capacity) return array;

   uint32_t new_capacity = capacity >= 6 ? (capacity + (capacity >> 1U)) : 8;
   if (new_capacity < count) new_capacity = count;

   return asetcap_(array, new_capacity, element_size);
}

void *acat_(void *array, void *other, uint32_t element_size) {
   if (!other) return array;
   if (!array) return other;

   auto len = ahdr(array)->len;
   auto other_len = ahdr(other)->len;
   array = afit_(array, len + other_len, element_size);
   memcpy((char *) array + len * element_size, other, other_len * element_size);
   ahdr(array)->len += other_len;
   return array;
}