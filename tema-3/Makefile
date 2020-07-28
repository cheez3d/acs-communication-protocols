DEBUG ?= 0
DEBUG_MARKER := .debug

CC := g++

CFLAGS := -std=c++1z -Wall -Wextra
CFLAGS_DEBUG := -DDEBUG -g
CFLAGS_NDEBUG := -DNDEBUG -Werror -O2 -flto

LDFLAGS := -Wall -Wextra
LDFLAGS_DEBUG := -g
LDFLAGS_NDEBUG := -O2 -Werror -flto

LDLIBS := 

INC_DIR := include
SRC_DIR := src
OBJ_DIR := obj

BIN := client
SRC :=$(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(subst $(SRC_DIR)/,$(OBJ_DIR)/,$(SRC:%.cpp=%.o))
DEP := $(OBJ:%.o=%.d)

$(shell mkdir -p $(INC_DIR) $(SRC_DIR) $(OBJ_DIR))

.DEFAULT_GOAL := build

ifneq ($(filter 0 valgrind,$(DEBUG) $(MAKECMDGOALS)),0)
    CFLAGS += $(CFLAGS_DEBUG)
    LDFLAGS += $(LDFLAGS_DEBUG)

    ifeq ($(wildcard $(DEBUG_MARKER)),)
        $(shell touch $(DEBUG_MARKER))
        $(shell touch Makefile)
    endif
else
    CFLAGS += $(CFLAGS_NDEBUG)
    LDFLAGS += $(LDFLAGS_NDEBUG)

    ifneq ($(wildcard $(DEBUG_MARKER)),)
        $(shell rm -f $(DEBUG_MARKER))
        $(shell touch Makefile)
    endif
endif

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $(BIN) $^

-include $(DEP)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile
	$(CC) -I $(INC_DIR) -MMD -c $(CFLAGS) -o $@ $<

.PHONY: build
build: $(BIN)

.PHONY: valgrind
valgrind: build
	@valgrind --tool=memcheck \
	          --leak-check=full \
	          --show-leak-kinds=all \
	          --track-origins=yes \
	          ./$(BIN) valgrind $(SV_ADDR) $(SV_PORT)

.PHONY: run
run: build
	@./$(BIN) $(SV_ADDR) $(SV_PORT)

.PHONY: clean
clean:
	@-rm -f $(OBJ) $(DEP) $(BIN)
