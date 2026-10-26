CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic -D_POSIX_C_SOURCE=199309L
RELEASE_FLAGS = -O2
DEBUG_FLAGS = -g -fsanitize=address,undefined -fno-omit-frame-pointer

INCLUDES = -Iinclude
SRC = src/main.c src/physics.c src/render.c src/input.c
OBJ = $(SRC:.c=.o)
TARGET = alchem-c

TEST_SRC = tests/test_reactions.c src/physics.c
TEST_TARGET = test_runner

.PHONY: all release debug test clean run

all: release

release: CFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

test: CFLAGS += $(DEBUG_FLAGS)
test: $(TEST_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TEST_TARGET) $^
	./$(TEST_TARGET)
	@rm -f $(TEST_TARGET)

run: release
	./$(TARGET)

clean:
	rm -f src/*.o $(TARGET) $(TEST_TARGET)
