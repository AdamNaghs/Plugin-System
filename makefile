# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -fPIC -Iinclude
LDFLAGS :=

# Detect Raylib, Lua, Python via Homebrew
RAYLIB_PREFIX := $(shell brew --prefix raylib 2>/dev/null)
LUA_PREFIX := $(shell brew --prefix lua 2>/dev/null)
PYTHON_PREFIX := $(shell brew --prefix python 2>/dev/null)

ifeq ($(RAYLIB_PREFIX),)
  RAYLIB_PREFIX := /usr/local
endif
ifeq ($(LUA_PREFIX),)
  LUA_PREFIX := /usr/local
endif
ifeq ($(PYTHON_PREFIX),)
  PYTHON_PREFIX := /usr/local
endif

# Raylib flags
RAYLIB_CFLAGS := -I$(RAYLIB_PREFIX)/include
RAYLIB_LDFLAGS := -L$(RAYLIB_PREFIX)/lib -lraylib -lm -ldl -lpthread

# Lua flags
LUA_CFLAGS := -I$(LUA_PREFIX)/include/lua
LUA_LDFLAGS := -L$(LUA_PREFIX)/lib -llua -lm -ldl

# Python flags
PY_CFLAGS := -I$(PYTHON_PREFIX)/Frameworks/Python.framework/Versions/3.13/include/python3.13
PY_LDFLAGS := -F$(PYTHON_PREFIX)/Frameworks -framework Python

# Source files
MAIN := src/main.c
TINYCTHREAD_SRC := external/tinycthread/source/tinycthread.c
CORE_SRCS := $(filter-out src/main.c, $(wildcard src/*.c))

GRAPHICS_SRC := plugins/graphics/graphics.c
SCHEDULER_SRC := plugins/scheduler/scheduler.c
SIGNALS_SRC := plugins/signals/signals.c
THREADS_SRC := plugins/threads/threads.c
LUA_SRC := plugins/lua/lua_plugin.c
PY_SRC := plugins/python/python_plugin.c
ENTITY_SRC := plugins/entity/entity.c
GAME_SRC := plugins/game/game.c

# Output binaries
TARGETS := build/test_runner \
           build/plugins/graphics.so \
           build/plugins/scheduler.so \
           build/plugins/signals.so \
           build/plugins/threads.so \
           build/plugins/lua.so \
           build/plugins/python.so \
           build/plugins/entity.so \
           build/plugins/game.so

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

build/plugins/lua.so: $(LUA_SRC) $(CORE_SRCS)
	$(CC) -shared $(CFLAGS) $(LUA_CFLAGS) $^ -o $@ $(LDFLAGS) $(LUA_LDFLAGS)

build/plugins/python.so: $(PY_SRC) $(CORE_SRCS)
	$(CC) -shared $(CFLAGS) $(PY_CFLAGS) $^ -o $@ $(LDFLAGS) $(PY_LDFLAGS)

build/plugins/entity.so: $(ENTITY_SRC) $(CORE_SRCS)
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS)

build/plugins/game.so: $(GAME_SRC) $(CORE_SRCS)
	$(CC) -shared $(CFLAGS) $(RAYLIB_CFLAGS) $^ -o $@ $(LDFLAGS) $(RAYLIB_LDFLAGS)

clean:
	rm -f $(TARGETS) *.o
