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
MAIN := src/main.c
TINYCTHREAD_SRC := external/tinycsthread/source/tinycthread.c
CORE_SRCS := $(filter-out src/main.c, $(wildcard src/*.c))
PLUGIN_API_SRC := plugins/raylib/test_plugin.c
SCHEDULER_SRC := plugins/scheduler/scheduler.c
SIGNALS_SRC := plugins/signals/signals.c

TARGETS := build/test_runner build/plugins/plugin_api.so build/plugins/scheduler.so build/plugins/signals.so
# Targets
all: $(TARGETS)

build/test_runner: $(MAIN) $(CORE_SRCS) 
	$(CC) $(CFLAGS) $^ -o $@ 

build/plugins/plugin_api.so: $(PLUGIN_API_SRC) $(CORE_SRCS) 
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS) 

build/plugins/scheduler.so: $(SCHEDULER_SRC) $(CORE_SRCS) 
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS) 

build/plugins/signals.so: $(SIGNALS_SRC) $(CORE_SRCS) 
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS) 

clean:
	rm -f $(TARGETS) *.o