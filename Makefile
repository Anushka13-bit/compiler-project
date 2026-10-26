CC      ?= gcc
FLEX    ?= flex
BISON   := $(shell command -v /opt/homebrew/opt/bison/bin/bison 2>/dev/null || \
                    command -v /usr/local/opt/bison/bin/bison 2>/dev/null || \
                    command -v bison)

CFLAGS  = -Wall -Wextra -std=c11 -Isrc -Ibuild -g
LDFLAGS =

SRC_DIR   = src
BUILD_DIR = build
BIN_DIR   = bin
BIN       = $(BIN_DIR)/resumec

C_SOURCES = ast.c diagnostics.c semantic.c codegen.c build.c job_matcher.c interactive.c main.c
OBJECTS   = $(C_SOURCES:%.c=$(BUILD_DIR)/%.o) $(BUILD_DIR)/lexer.o $(BUILD_DIR)/parser.tab.o

.PHONY: all clean test

all: $(BIN)

$(BIN): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

$(BUILD_DIR)/parser.tab.c $(BUILD_DIR)/parser.tab.h: $(SRC_DIR)/parser.y | $(BUILD_DIR)
	$(BISON) -d -Wall -o $(BUILD_DIR)/parser.tab.c $(SRC_DIR)/parser.y

$(BUILD_DIR)/lexer.c: $(SRC_DIR)/lexer.l $(BUILD_DIR)/parser.tab.h | $(BUILD_DIR)
	$(FLEX) -o $(BUILD_DIR)/lexer.c $(SRC_DIR)/lexer.l

$(BUILD_DIR)/parser.tab.o: $(BUILD_DIR)/parser.tab.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/lexer.o: $(BUILD_DIR)/lexer.c
	$(CC) $(CFLAGS) -Wno-sign-compare -Wno-unused-function -c -o $@ $<

$(BUILD_DIR)/main.o: $(BUILD_DIR)/parser.tab.h

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

test: all
	bash tests/run_tests.sh
