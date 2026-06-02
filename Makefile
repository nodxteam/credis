CC     = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude

SRCS = \
  src/core/main.c          \
  src/core/server.c        \
  src/core/config.c        \
  src/core/thread_pool.c   \
  src/net/io.c             \
  src/store/db.c           \
  src/store/rlist.c        \
  src/store/rhash.c        \
  src/store/rset.c         \
  src/resp/writer.c        \
  src/resp/parser.c        \
  src/persist/aof.c        \
  src/commands/dispatcher.c \
  src/commands/generic.c   \
  src/commands/string_cmds.c \
  src/commands/list_cmds.c \
  src/commands/hash_cmds.c \
  src/commands/set_cmds.c

ifeq ($(OS),Windows_NT)
    TARGET   = credis.exe
    LDFLAGS  = -lws2_32
else
    TARGET   = credis
    LDFLAGS  = -lpthread
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f credis credis.exe credis.aof
