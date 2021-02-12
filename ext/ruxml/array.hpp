#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstddef>

#include "memory.hpp"

struct ArrayHeader {
   Allocator *allocator;
   uint32_t len;
   uint32_t cap;
   char *buffer[1];
};

#define ahdr(array) ((ArrayHeader *)((char *)(array) - offsetof(ArrayHeader, buffer)))
#define alen(array) ((array) ? ahdr((array))->len : 0)
#define acap(array) ((array) ? ahdr((array))->cap : 0)

void aclear(void *array);
void *ainit_(void *array, Allocator *allocator = nullptr);
void afree_(void *array);
void *afit_(void *array, uint32_t count, uint32_t element_size);
void *asetcap_(void *array, uint32_t new_cap, uint32_t element_size);
void *acat_(void *array, void *other, uint32_t element_size);

#define ainit(array, ...) ((array) = (decltype(array))ainit_(array, __VA_ARGS__))
#define afree(array) (afree_((array)), (array) = nullptr)
#define asetcap(array, new_cap) ((array) = (decltype(array))asetcap_((array), (new_cap), sizeof(*(array))))
#define ashrink(array) ((array) = (decltype(array))asetcap_((array), alen((array)), sizeof(*(array))))
#define asetlen(array, new_len) ((array) = (decltype(array))afit_((array), new_len, sizeof(*array)), ahdr(array)->len = new_len)
#define aempty(array) (ahdr(array)->len = 0)
#define apush(array, item) ((array) = (decltype(array))afit_((array), alen(array) + 1, sizeof(*array)), array[ahdr(array)->len++] = (item))
#define adel(array, index) ((array)[(index)] = (array)[--ahdr((array))->len])
#define acat(array, other) ((array) = (decltype(array))acat_((array), (other), sizeof(*array)))