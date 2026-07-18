MANDIR ?= $(HOME)/.local/share/man/man1

VERSION := $(shell git describe --tags --match 'v*' --always --dirty 2>/dev/null)
ifeq ($(VERSION),)
VERSION := v$(shell cat VERSION 2>/dev/null || echo unknown)
endif

SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
OUT = pm

LUA_CFLAGS = $(shell pkg-config --cflags lua 2>/dev/null || pkg-config --cflags lua5.4 2>/dev/null)
LUA_LIBS = $(shell pkg-config --libs lua 2>/dev/null || pkg-config --libs lua5.4 2>/dev/null || echo '-llua')
LIBS = $(LUA_LIBS) -lcrypto -lcurl
BASE_FLAGS = $(LUA_CFLAGS) $(LIBS) -std=c99 -DVERSION='"$(VERSION)"'
CC = clang
RM = rm -rf

DEBUG_FLAGS = -O0 -Wall -Wextra -ggdb -fsanitize=address,null,undefined
RELEASE_FLAGS = -O2 -Wall -Wextra

all: release $(MANDIR)/pm.1

release: FLAGS = $(BASE_FLAGS) $(RELEASE_FLAGS)
release: clean
release: $(OUT)

debug: FLAGS = $(BASE_FLAGS) $(DEBUG_FLAGS)
debug: $(OUT)

$(OUT): $(SRC) $(HEADERS)
	$(CC) $(FLAGS) $(SRC) -o $@

appimage: release
	cp $(OUT) AppDir/usr/bin/pm

clean:
	$(RM) $(OUT)

cleanall:
	$(RM) $(OUT) ~/.local/share/pm ~/.local/bin/pm ~/.local/state/pm .cache

install: $(MANDIR)/pm.1

$(MANDIR)/pm.1: pm.1
	mkdir -p $(MANDIR)
	sed 's/@VERSION@/$(VERSION)/' pm.1 > $@
