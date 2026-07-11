SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
OUT = pm
MANDIR ?= $(HOME)/.local/share/man/man1
BASE_FLAGS = $(LIBS) -std=c99
LIBS = -llua -lcrypto -lcurl
CC = clang
RM = rm -rf

DEBUG_FLAGS = -O0 -Wall -Wextra -ggdb -fsanitize=address,null,undefined
RELEASE_FLAGS = -O2 -Wall -Wextra

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

install: pm.1
	mkdir -p $(MANDIR)
	cp pm.1 $(MANDIR)/pm.1
