#ifdef DEBUG
#define debugf(...) do {                                           \
    fprintf(stderr, "%s:%s:%d: ", __FILE__, __func__, __LINE__);  \
    fprintf(stderr, __VA_ARGS__);                                 \
    fflush(stderr);                                               \
} while (0)

#define assert(c) if (!(c)) __builtin_trap()

#else
#define debugf(...) ((void)0)
#define assert(...)((void)0)
#endif
