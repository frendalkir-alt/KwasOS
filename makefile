# Makefile для 8080 VM с SDL2

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./src -I./src/gui -I./src/devices
LDFLAGS = -lSDL2 -lSDL2_ttf -lm

SRC_DIR = src
GUI_DIR = $(SRC_DIR)/gui
DEV_DIR = $(SRC_DIR)/devices

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/cpu.c \
       $(SRC_DIR)/memory.c \
       $(GUI_DIR)/window.c \
       $(GUI_DIR)/terminal.c \
       $(GUI_DIR)/debug.c \
       $(DEV_DIR)/keyboard.c

OBJS = $(SRCS:.c=.o)
TARGET = 8080-vm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean