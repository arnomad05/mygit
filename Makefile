CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -g -I./include -pipe
LDFLAGS :=

ifdef ComSpec
    EXE_EXT := .exe
    CFLAGS += -D_WIN32
else
    UNAME_S := $(shell uname -s)
    ifneq ($(findstring MINGW,$(UNAME_S)),)
        EXE_EXT := .exe
        CFLAGS += -D_WIN32
    else ifneq ($(findstring MSYS,$(UNAME_S)),)
        EXE_EXT := .exe
        CFLAGS += -D_WIN32
    else
        EXE_EXT :=
    endif
endif

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
EXECUTABLE := $(BIN_DIR)/mygit$(EXE_EXT)

.PHONY: all clean rebuild run

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	mkdir -p $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build successful: $(EXECUTABLE)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

rebuild: clean all

run: $(EXECUTABLE)
	./$(EXECUTABLE) --help
