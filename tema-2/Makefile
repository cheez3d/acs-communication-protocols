SV_ADDR := 127.0.0.1
SV_PORT := 25565

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

SRC_DIR_SV := $(SRC_DIR)/server
OBJ_DIR_SV := $(subst $(SRC_DIR)/,$(OBJ_DIR)/,$(SRC_DIR_SV))

SRC_DIR_CL := $(SRC_DIR)/subscriber
OBJ_DIR_CL := $(subst $(SRC_DIR)/,$(OBJ_DIR)/,$(SRC_DIR_CL))

SRC :=$(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(subst $(SRC_DIR)/,$(OBJ_DIR)/,$(SRC:%.cpp=%.o))
DEP := $(OBJ:%.o=%.d)

BIN_SV := server
SRC_SV := $(wildcard $(SRC_DIR_SV)/*.cpp)
OBJ_SV := $(subst $(SRC_DIR_SV)/,$(OBJ_DIR_SV)/,$(SRC_SV:%.cpp=%.o))
DEP_SV := $(OBJ_SV:%.o=%.d)

BIN_CL := subscriber
SRC_CL := $(wildcard $(SRC_DIR_CL)/*.cpp)
OBJ_CL := $(subst $(SRC_DIR_CL)/,$(OBJ_DIR_CL)/,$(SRC_CL:%.cpp=%.o))
DEP_CL := $(OBJ_CL:%.o=%.d)

$(shell mkdir -p $(INC_DIR) $(SRC_DIR) $(OBJ_DIR))
$(shell mkdir -p $(SRC_DIR_SV) $(OBJ_DIR_SV))
$(shell mkdir -p $(SRC_DIR_CL) $(OBJ_DIR_CL))

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

$(BIN_SV): $(OBJ) $(OBJ_SV)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $(BIN_SV) $^

$(BIN_CL): $(OBJ) $(OBJ_CL)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $(BIN_CL) $^

-include $(DEP)
-include $(DEP_SV)
-include $(DEP_CL)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile
	$(CC) -iquote $(INC_DIR) -MMD -c $(CFLAGS) -o $@ $<

$(OBJ_DIR_SV)/%.o: $(SRC_DIR_SV)/%.cpp Makefile
	$(CC) -iquote $(INC_DIR) -MMD -c $(CFLAGS) -o $@ $<

$(OBJ_DIR_CL)/%.o: $(SRC_DIR_CL)/%.cpp Makefile
	$(CC) -iquote $(INC_DIR) -MMD -c $(CFLAGS) -o $@ $<

.PHONY: build_sv
build_sv: $(BIN_SV)

.PHONY: build_cl
build_cl: $(BIN_CL)

.PHONY: build
build: build_sv build_cl

.PHONY: valgrind_sv
valgrind_sv: build
	@valgrind --tool=memcheck \
	          --leak-check=full \
	          --show-leak-kinds=all \
	          --track-origins=yes \
	          ./$(BIN_SV) $(SV_PORT)

.PHONY: valgrind_cl
valgrind_cl: build
	@valgrind --tool=memcheck \
	          --leak-check=full \
	          --show-leak-kinds=all \
	          --track-origins=yes \
	          ./$(BIN_CL) "$@" $(SV_ADDR) $(SV_PORT)

.PHONY: run_sv
run_sv: build_sv
	@./$(BIN_SV) $(SV_PORT)

.PHONY: clean
clean:
	@-rm -f $(OBJ) $(OBJ_SV) $(OBJ_CL) $(DEP) $(DEP_SV) $(DEP_CL) $(BIN_SV) $(BIN_CL)
