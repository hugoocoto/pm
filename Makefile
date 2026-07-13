SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
OUT = pm
MANDIR ?= $(HOME)/.local/share/man/man1
VERSION := $(shell git describe --tags --always --dirty 2>/dev/null)
ifeq ($(VERSION),)
VERSION := v$(shell cat VERSION 2>/dev/null || echo unknown)
endif
BASE_FLAGS = $(LIBS) -std=c99 -DVERSION='"$(VERSION)"'
LIBS = -llua -lcrypto -lcurl
CC = clang
RM = rm -rf

DEBUG_FLAGS = -O0 -Wall -Wextra -ggdb -fsanitize=address,null,undefined
RELEASE_FLAGS = -O2 -Wall -Wextra

all: release $(MANDIR)/pm.1

release: FLAGS = $(BASE_FLAGS) $(RELEASE_FLAGS)
release: $(OUT)

debug: FLAGS = $(BASE_FLAGS) $(DEBUG_FLAGS)
debug: $(OUT)

$(OUT): $(SRC) $(HEADERS)
	$(CC) $(FLAGS) $(SRC) -o $@

clean:
	$(RM) $(OUT)

cleanall:
	$(RM) $(OUT) ~/.local/share/pm ~/.local/bin/pm ~/.local/state/pm .cache

install: $(MANDIR)/pm.1

$(MANDIR)/pm.1: pm.1
	mkdir -p $(MANDIR)
	sed 's/@VERSION@/$(VERSION)/' pm.1 > $@
