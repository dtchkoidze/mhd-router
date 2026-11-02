#pragma once
#include "integers.h"
#include <stdbool.h>
#include <stddef.h>

// for string literals https://nullprogram.com/blog/2023/01/18/#implementation-highlights
#define s8_lit(string) s8_make((string), (i64)(sizeof(string) - 1))
#define s8_spread(s) (int)s.len, s.data

typedef struct s8 {
    const u8 *data;
    i64 len; // jai=>valigo=>here, if underflows u kno shit happened
} s8;

static inline s8 s8_make(const void *data, i64 len) {
    if (len < 0 || (data == NULL && len != 0)) {
        return (s8){0};
    }

    return (s8){
        .data = (const u8 *)data,
        .len = len,
    };
}

static inline bool s8_empty(const s8 *s) {
    if (s == NULL) {
        return true;
    }

    if (s->len <= 0) {
        return true;
    }

    return false;
}

bool s8_eq(const s8 *a, const s8 *b);
s8 s8_from_cstring(const char *string);
s8 s8_at(const s8 *s, i64 start, i64 end);
s8 s8_trimspace(const s8 *s, u8 *output, i64 output_capacity);
s8 s8_trim(const s8 *s);
