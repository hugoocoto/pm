SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
OUT = pm
FLAGS = -Wall -Wextra -ggdb $(LIBS) -std=c99 -D_DEFAULT_SOURCE
LIBS = -llua -lcrypto -lcurl
CC = cc
RM = rm -rf

$(OUT): $(SRC)
	$(CC) $(FLAGS) $^ -o $@

$(OUT): $(HEADERS)

clean: $(OUT)
	$(RM) $^


