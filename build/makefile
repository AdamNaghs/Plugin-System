# Makefile for building test_runner and plugin_api.so

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -fPIC -Iinclude -Iexternal/C-Collection-Map
LDFLAGS :=

# Try to detect raylib via Homebrew
RAYLIB_PREFIX := $(shell brew --prefix raylib 2>/dev/null)

ifeq ($(RAYLIB_PREFIX),)
  RAYLIB_INCLUDE := /usr/local/include
  RAYLIB_LIB := /usr/local/lib
else
  RAYLIB_INCLUDE := $(RAYLIB_PREFIX)/include
  RAYLIB_LIB := $(RAYLIB_PREFIX)/lib
endif

CFLAGS += -I$(RAYLIB_INCLUDE)
LDFLAGS += -L$(RAYLIB_LIB) -lraylib -lm -ldl -lpthread

# Source files
CORE_SRCS := $(wildcard src/*.c)
MAP_SRCS := external/C-Collection-Map/map.c
TEST_SRCS := tests/test.c
PLUGIN_API_SRC := tests/plugin_api.c

# Targets
all: test_runner plugins/plugin_api.so

test_runner: $(TEST_SRCS) $(CORE_SRCS) $(MAP_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ -g

plugins/plugin_api.so: $(PLUGIN_API_SRC) $(CORE_SRCS) $(MAP_SRCS)
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS) -g

clean:
	rm -f build/test_runner build/plugins/plugin_api.so *.o