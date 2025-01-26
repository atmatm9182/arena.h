#include <stdio.h>

#define ARENA_H_IMPLEMENTATION
#include "arena.h"

int main() {
    Arena a = {0};
    int* i = arena_alloc(&a, sizeof(int));
    char* str = arena_alloc(&a, sizeof(char) * 10);

    printf("i: %p\n", i);
    printf("str: %p\n", str);

    *i = 68;

    sprintf(str, "%d", *i);
    printf("%d\n", *i);
    return 0;
}
