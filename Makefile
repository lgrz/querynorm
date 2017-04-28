TARGET = querynorm

CC = gcc
CFLAGS += -std=c99 -Wall -Wextra -pedantic -O2 -D_XOPEN_SOURCE=700 \
		  -Isrc
LDFLAGS += -lm
DEBUG_CFLAGS = -g -O0 -DDEBUG

SRC = src/main.c src/util.c
OBJ := $(SRC:.c=.o)
DEP := $(patsubst %.c,%.d,$(SRC))

.PHONY: all
all: $(TARGET)

.PHONY: debug
debug: CFLAGS := $(CFLAGS:-O%=)
debug: CC := $(CC) $(DEBUG_CFLAGS)
debug: all

.PHONY: release
release: all

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJ) $(DEP)

-include $(DEP)
