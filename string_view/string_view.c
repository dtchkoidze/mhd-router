#include "string_view.h"
#include "../base/integers.h"
#include <string.h>
#include "../base/macros.h"

typedef struct split_half_res {
    s8 a;
    s8 b;
} split_half_res;

static bool s8_is_whitespace(u8 byte) {
    return byte == ' ' || byte == '\t' || byte == '\n' ||
           byte == '\v' || byte == '\f' || byte == '\r';
}

s8 s8_trimspace(const s8 *s, u8 *output, i64 output_capacity) {
    if (s == NULL || s->len < 0 || output_capacity < 0 ||
        (s->data == NULL && s->len != 0)) {
        return (s8){0};
    }

    i64 output_length = 0;
    for (i64 i = 0; i < s->len; i += 1) {
        if (!s8_is_whitespace(s->data[i])) {
            output_length += 1;
        }
    }

    if (output_length > output_capacity ||
        (output == NULL && output_length != 0)) {
        return (s8){0};
    }

    i64 output_index = 0;
    for (i64 i = 0; i < s->len; i += 1) {
        if (!s8_is_whitespace(s->data[i])) {
            output[output_index++] = s->data[i];
        }
    }

    return s8_make(output, output_length);
}

s8 s8_trim(const s8 *s) {
    if (s == NULL || s->len < 0 ||
        (s->data == NULL && s->len != 0)) {
        return (s8){0};
    }
    if (s->len == 0) {
        return *s;
    }

    i64 start = 0;
    while (start < s->len && s8_is_whitespace(s->data[start])) {
        start += 1;
    }

    i64 end = s->len;
    while (end > start && s8_is_whitespace(s->data[end - 1])) {
        end -= 1;
    }

    return s8_make(s->data + start, end - start);
}

split_half_res s8_split_in_half(const s8 *s) {
    if (s == NULL) {
        return (split_half_res){0};
    }

    if ((s->len & 1) != 0) { // cant split in half
        return (split_half_res){0};
    }

    // tes tin
    // 012 345
    i64 midpoint = (s->len / 2);
    return (split_half_res){
        .a = s8_at(s, 0, midpoint),
        .b = s8_at(s, midpoint, s->len),
    };
}

// start including, end excluding
s8 s8_at(const s8 *s, i64 start, i64 end) {
    if (s == NULL || start < 0 || end < start || end > s->len) {
        return (s8){0};
    }

    return s8_make(s->data + start, end - start);
}

bool s8_eq(const s8 *a, const s8 *b) {
    if (a == NULL || b == NULL) {
        return false;
    }

    if (a->len != b->len) {
        return false;
    }

    if (a->data == b->data) { // literally same pointers
        return true;
    }

    for (i64 i = 0; i < a->len; i++) {
        if (a->data[i] != b->data[i]) {
            return false;
        }
    }

    return true;
}

s8 s8_from_cstring(const char *string) {
    if (string == NULL) {
        return (s8){0};
    }

    return s8_make(string, (i64)strlen(string));
}
