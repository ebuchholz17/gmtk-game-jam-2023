#ifndef LIST_H
#define LIST_H

#include "gng_memory.h"

#define LIST_TYPE_STRUCT(x) LIST_TYPE_STRUCT_(x)
#define LIST_TYPE_STRUCT_(x) x##_list
#define LIST_INIT(x) LIST_INIT_(x)
#define LIST_INIT_(x) x##_listInit
#define LIST_PUSH(x) LIST_PUSH_(x)
#define LIST_PUSH_(x) x##_listPush
#define LIST_POP(x) LIST_POP_(x)
#define LIST_POP_(x) x##_listPop
#define LIST_INSERT(x) LIST_INSERT_(x)
#define LIST_INSERT_(x) x##_listInsert
#define LIST_SPLICE(x) LIST_SPLICE_(x)
#define LIST_SPLICE_(x) x##_listSplice
#define LIST_COPY(x) LIST_COPY_(x)
#define LIST_COPY_(x) x##_listCopy
#define LIST_CLONE(x) LIST_CLONE_(x)
#define LIST_CLONE_(x) x##_listClone
#define LIST_REVERSE(x) LIST_REVERSE_(x)
#define LIST_REVERSE_(x) x##_listReverse

#endif

typedef struct LIST_TYPE_STRUCT(LIST_TYPE) {
    LIST_TYPE *values;
    u32 numValues;
    u32 capacity;
} LIST_TYPE_STRUCT(LIST_TYPE);

void LIST_PUSH(LIST_TYPE) (LIST_TYPE_STRUCT(LIST_TYPE) *list, LIST_TYPE value) {
    ASSERT(list->numValues < list->capacity);

    list->values[list->numValues] = value;
    list->numValues++;
}

LIST_TYPE LIST_POP(LIST_TYPE) (LIST_TYPE_STRUCT(LIST_TYPE) *list) {
    ASSERT(list->numValues >= 0);

    LIST_TYPE value = list->values[list->numValues - 1];
    list->numValues--;
    return value;
}

void LIST_INSERT(LIST_TYPE) (LIST_TYPE_STRUCT(LIST_TYPE) *list, LIST_TYPE value, u32 index) {
    ASSERT(index <= list->numValues);
    ASSERT(list->numValues < list->capacity);

    for (u32 i = list->numValues - 1; i >= index; --i) {
        list->values[i+1] = list->values[i];
    }
    list->values[index] = value;
    list->numValues++;
}

LIST_TYPE LIST_SPLICE(LIST_TYPE) (LIST_TYPE_STRUCT(LIST_TYPE) *list, u32 index) {
    ASSERT(index < list->numValues);

    LIST_TYPE result = list->values[index];
    list->numValues--;
    for (u32 i = index; i < list->numValues; ++i) {
        list->values[i] = list->values[i+1];
    }

    return result;
}

LIST_TYPE_STRUCT(LIST_TYPE) LIST_COPY(LIST_TYPE) (LIST_TYPE_STRUCT(LIST_TYPE) *source, LIST_TYPE *memory) {
    LIST_TYPE_STRUCT(LIST_TYPE) result = {0};
    result.numValues = source->numValues;
    result.capacity = source->capacity;
    result.values = memory;
    for (u32 i = 0; i < source->numValues; ++i) {
        result.values[i] = source->values[i];
    }

    return result;
}

void LIST_CLONE(LIST_TYPE) (LIST_TYPE_STRUCT(LIST_TYPE) *source, LIST_TYPE_STRUCT(LIST_TYPE) *dest) {
    dest->numValues = source->numValues;
    for (u32 i = 0; i < source->numValues; ++i) {
        dest->values[i] = source->values[i];
    }
}

void LIST_REVERSE(LIST_TYPE) (LIST_TYPE_STRUCT(LIST_TYPE) *list) {
    for (u32 i = 0; i < list->numValues / 2; ++i) {
        LIST_TYPE val = list->values[i];
        LIST_TYPE otherVal = list->values[list->numValues - i - 1];
        list->values[i] = otherVal;
        list->values[list->numValues - i - 1] = val;
    }
}

LIST_TYPE_STRUCT(LIST_TYPE) LIST_INIT(LIST_TYPE) (mem_arena *memory, u32 capacity) {
    LIST_TYPE_STRUCT(LIST_TYPE) result = {0};
    result.capacity = capacity;
    result.values = (LIST_TYPE *)allocMemory(memory, sizeof(LIST_TYPE) * capacity);
    return result;
}

#undef LIST_TYPE
