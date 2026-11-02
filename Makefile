.PHONY: build build_debug

SOURCES := \
	string_view/string_view.c \
	cmd/main.c \
	router/router.c \
	router/util.c \
	router/http.c \
	router/rax.c \
	router/rc4rand.c \
	router/crc16.c \


LIBS := $(shell pkg-config --cflags --libs libmicrohttpd libpq) -lm

build_debug:
	mkdir -p bin
	clang -DDEBUG=1 -g3 -O0 $(SOURCES) -o bin/main_debug $(LIBS)

build:
	mkdir -p bin
	clang -O2 $(SOURCES) -o bin/main $(LIBS)
