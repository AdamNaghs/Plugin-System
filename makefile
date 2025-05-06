# Makefile for building test_runner and plugin_api.so

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -fPIC -Iinclude 
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

RAYLIB_CFLAGS += -I$(RAYLIB_INCLUDE) 
RAYLIB_LDFLAGS += -L$(RAYLIB_LIB) -lraylib -lm -ldl -lpthread

# Source files
MAIN := src/main.c
TINYCTHREAD_SRC := external/tinycthread/source/tinycthread.c
CORE_SRCS := $(filter-out src/main.c, $(wildcard src/*.c))
GRAPHICS_SRC := plugins/raylib/graphics.c
SCHEDULER_SRC := plugins/scheduler/scheduler.c
SIGNALS_SRC := plugins/signals/signals.c
THREADS_SRC := plugins/threads/threads.c

TARGETS := build/test_runner build/plugins/graphics.so build/plugins/scheduler.so build/plugins/signals.so build/plugins/threads.so
# Targets
all: $(TARGETS)

build/test_runner: $(MAIN) $(CORE_SRCS) 
	$(CC) $(CFLAGS) $^ -o $@ 

build/plugins/graphics.so: $(GRAPHICS_SRC) $(CORE_SRCS) 
	$(CC) -shared $(CFLAGS) $(RAYLIB_CFLAGS) $^ -o $@ $(LDFLAGS) $(RAYLIB_LDFLAGS)

build/plugins/scheduler.so: $(SCHEDULER_SRC) $(CORE_SRCS) 
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS) 

build/plugins/signals.so: $(SIGNALS_SRC) $(CORE_SRCS) 
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS) 

build/plugins/threads.so: $(THREADS_SRC) $(CORE_SRCS) $(TINYCTHREAD_SRC)
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS) 

clean:
	rm -f $(TARGETS) *.o