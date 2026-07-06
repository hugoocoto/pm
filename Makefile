SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
OUT = pm
FLAGS = -Wall -Wextra -ggdb $(LIBS) -std=c99
LIBS = -llua -lcrypto -lcurl
CC = cc
RM = rm -rf

$(OUT): $(SRC)
	$(CC) $(FLAGS) $^ -o $@

$(OUT): $(HEADERS)

clean: $(OUT)
	$(RM) $^

cleanall:
	$(RM) $(OUT) ~/.local/share/pm ~/.local/bin/pm ~/.local/state/pm .cache
