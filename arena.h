#ifndef ARENA_H_
#define ARENA_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct ArenaRegion {
    struct ArenaRegion* next;
    void* data;
    size_t size;
    size_t off;
} ArenaRegion;

typedef struct {
    ArenaRegion* head;
    ArenaRegion* region_pool;
} Arena;

void* arena_alloc(Arena*, size_t);
void* arena_zalloc(Arena*, size_t);
void arena_free(Arena*);
void* arena_realloc(Arena* a, void* ptr, size_t old_sz, size_t new_sz);
void arena_reserve(Arena*, size_t);
void arena_destroy(Arena*);

size_t arena_avail(const Arena*);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef ARENA_H_IMPLEMENTATION

#if defined(__unix__) || defined(__unix) || defined(__APPLE__)

#include <sys/mman.h>
#include <unistd.h>

#define ARENA_ALLOC_PAGE(sz) (mmap(NULL, (sz), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0))
#define ARENA_ALLOC_SMOL(sz) (sbrk((sz)))

#else

#error "Only UNIX-like systems are supported"

#endif // unix check

#ifndef ARENA_ASSERT

#include <assert.h>
#define ARENA_ASSERT assert

#endif // ARENA_ASSSERT

#include <string.h>

#define ARENA_PAGE_SIZE 4096

static inline uintptr_t arena_align_ptr(uintptr_t size, uintptr_t align) {
    return size + ((align - (size & (align - 1))) & (align - 1));
}

ArenaRegion* arena_alloc_region(Arena* arena, size_t size) {
    size = arena_align_ptr(size, ARENA_PAGE_SIZE);

    ArenaRegion* head = arena->region_pool;

    while (head) {
        if (head->size - head->off >= sizeof(ArenaRegion)) {
            ArenaRegion* region = (ArenaRegion*)((uint8_t*)head->data + head->off);

            region->data = ARENA_ALLOC_PAGE(size);
            ARENA_ASSERT(region->data != (void*)-1);
            region->size = size;
            region->off = 0;
            region->next = NULL;

            return region;
        }

        head = head->next;
    }

    head = (ArenaRegion*)ARENA_ALLOC_SMOL(sizeof(ArenaRegion));
    head->next = arena->region_pool;
    arena->region_pool = head;

    head->data = ARENA_ALLOC_PAGE(size);
    head->size = size;
    head->off = 0;
    head->next = NULL;
    return head;
}

void* arena_alloc(Arena* arena, size_t sz) {
    ArenaRegion* head = arena->head;

    sz = arena_align_ptr(sz, sizeof(void*));

    while (head) {
        if (head->size - head->off >= sz) {
            void* ptr = (void*)((uint8_t*)head->data + head->off);
            head->off += sz;
            return ptr;
        }

        head = head->next;
    }

    head = arena_alloc_region(arena, arena_align_ptr(sz, ARENA_PAGE_SIZE));
    head->next = arena->head;
    arena->head = head;

    void* ptr = (void*)((uint8_t*)head->data + head->off);
    head->off += sz;
    return ptr;
}

void* arena_zalloc(Arena* arena, size_t sz) {
    void* ptr = arena_alloc(arena, sz);
    memset(ptr, 0, sz);
    return ptr;
}

void arena_free(Arena* arena) {
    ArenaRegion* head = arena->head;
    while (head) {
        head->off = 0;
        head = head->next;
    }
}

void* arena_realloc(Arena* arena, void* ptr, size_t old_sz, size_t new_sz) {
    void* new_ptr = arena_alloc(arena, new_sz);
    memcpy(new_ptr, ptr, old_sz);
    return new_ptr;
}

void arena_reserve(Arena* arena, size_t sz) {
    size_t free = 0;

    ArenaRegion* head = arena->head;
    while (head) {
        free += head->size - head->off;
        head = head->next;
    }

    if (free >= sz) {
        return;
    }

    head = arena_alloc_region(arena, arena_align_ptr(sz - free, ARENA_PAGE_SIZE));
    head->next = arena->head;
    arena->head = head;
}

void arena_destroy(Arena* arena) {
    ArenaRegion* head = arena->head;

    while (head) {
        ARENA_ASSERT(munmap(head->data, head->size) == 0);
        head = head->next;
    }
}

size_t arena_avail(const Arena* a) {
    ArenaRegion* head = a->head;

    size_t res = 0;
    while (head) {
        res += head->size - head->off;
        head = head->next;
    }

    return res;
}

#endif // ARENA_H_IMPLEMENTATION

#endif // ARENA_H_
