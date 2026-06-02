# credis

A Redis-compatible in-memory key-value store written in C from scratch.
Implements the RESP (Redis Serialization Protocol), making it compatible with
`redis-cli` and any standard Redis client library. Built as a learning project
covering network programming, data structures, protocol design, and persistence.

## Features

- RESP protocol — multi-bulk and inline command formats
- Four data types: string, list, hash, set
- AOF (Append Only File) persistence with startup replay
- TTL / key expiry — lazy expiry on access + active background cleanup
- Thread-per-connection model with a configurable thread pool
- Compatible with `redis-cli` and standard Redis client libraries

## Supported commands

### Generic
| Command | Description |
|---------|-------------|
| `PING [msg]` | Connectivity check |
| `DEL key [key ...]` | Delete one or more keys |
| `EXISTS key [key ...]` | Check if keys exist |
| `TYPE key` | Return the type of a key |
| `EXPIRE key seconds` | Set a TTL in seconds |
| `EXPIREAT key timestamp` | Set expiry as a Unix timestamp |
| `TTL key` | Remaining TTL in seconds (-1 = no expiry, -2 = not found) |
| `PERSIST key` | Remove the TTL from a key |
| `KEYS pattern` | List keys matching a glob pattern |
| `DBSIZE` | Number of keys in the database |
| `FLUSHDB` | Delete all keys |
| `QUIT` | Close the connection |

### String
| Command | Description |
|---------|-------------|
| `SET key value [EX sec] [NX\|XX]` | Set a string value |
| `GET key` | Get a string value |
| `GETSET key value` | Set and return the old value |
| `SETNX key value` | Set only if key does not exist |
| `SETEX key seconds value` | Set with expiry |
| `MSET key value [...]` | Set multiple keys |
| `MGET key [key ...]` | Get multiple keys |
| `INCR key` | Increment integer value by 1 |
| `INCRBY key n` | Increment by n |
| `DECR key` | Decrement by 1 |
| `DECRBY key n` | Decrement by n |
| `APPEND key value` | Append to a string |
| `STRLEN key` | Length of a string |

### List
| Command | Description |
|---------|-------------|
| `LPUSH key el [el ...]` | Prepend elements |
| `RPUSH key el [el ...]` | Append elements |
| `LPOP key` | Remove and return head |
| `RPOP key` | Remove and return tail |
| `LRANGE key start stop` | Get a slice (negative indices supported) |
| `LLEN key` | Length of the list |
| `LINDEX key index` | Get element at index |
| `LSET key index value` | Set element at index |
| `LREM key count value` | Remove occurrences of value |
| `LTRIM key start stop` | Trim list to a range |

### Hash
| Command | Description |
|---------|-------------|
| `HSET key field value [...]` | Set one or more fields |
| `HGET key field` | Get a field value |
| `HMGET key field [field ...]` | Get multiple field values |
| `HGETALL key` | Get all fields and values |
| `HDEL key field [field ...]` | Delete fields |
| `HEXISTS key field` | Check if a field exists |
| `HLEN key` | Number of fields |
| `HKEYS key` | All field names |
| `HVALS key` | All field values |
| `HINCRBY key field n` | Increment a field by n |

### Set
| Command | Description |
|---------|-------------|
| `SADD key member [member ...]` | Add members |
| `SREM key member [member ...]` | Remove members |
| `SMEMBERS key` | All members |
| `SISMEMBER key member` | Check membership |
| `SCARD key` | Number of members |
| `SUNION key [key ...]` | Union of sets |
| `SINTER key [key ...]` | Intersection of sets |
| `SDIFF key [key ...]` | Difference of sets |

## Project structure

```
credis/
├── include/
│   ├── platform.h          cross-platform socket/thread abstractions
│   ├── net/io.h            buffered IO context
│   ├── core/               server, config, thread pool
│   ├── store/              db (hash table), rlist, rhash, rset
│   ├── resp/               RESP parser and writer
│   ├── commands/           dispatcher and per-type command headers
│   └── persist/            AOF persistence
├── src/
│   └── (mirrors include/)
├── credis.conf
└── Makefile
```

### Request pipeline

```
accept()
  |
  +-- pool_submit: hand connection to a worker thread
        |
        +-- resp/parser.c: read buffered bytes, parse RESP command
        |
        +-- commands/dispatcher.c: case-insensitive command lookup
        |     arity check
        |     dispatch to handler function
        |
        +-- commands/{type}_cmds.c: acquire db->lock, operate on store
        |
        +-- resp/writer.c: write RESP response to client
        |
        +-- persist/aof.c: if write command, append to AOF file
        |
        +-- loop: read next command (keep-alive)
```

### Data model

The main database is a hash table of `KVEntry` objects, each holding a type tag,
a pointer to the actual value, and an optional expiry timestamp.

```
DB (hash table, 4096 buckets, chaining)
  KVEntry { key, type, value*, expire_at }
    RTYPE_STRING → char*
    RTYPE_LIST   → RList  (doubly linked list of char*)
    RTYPE_HASH   → RHash  (hash table of field → value)
    RTYPE_SET    → RSet   (hash table, membership only)
```

All DB operations take `db->lock` (mutex) before reading or writing. Expiry is
handled in two ways:

- **Lazy expiry**: `db_find()` returns NULL for expired keys without deleting them.
- **Active expiry**: a background thread calls `db_purge_expired()` at the
  rate configured by `hz` (default 10 times per second).

### RESP protocol

credis speaks the Redis Serialization Protocol (RESP2). Both multi-bulk and
inline command formats are supported:

```
# Multi-bulk (standard)
*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n

# Inline (for quick manual testing)
SET foo bar\r\n
```

Responses follow the same protocol:
```
+OK\r\n                    simple string
-ERR message\r\n           error
:1000\r\n                  integer
$6\r\nfoobar\r\n           bulk string
$-1\r\n                    null bulk string
*2\r\n$3\r\nfoo\r\n...     array
```

### AOF persistence

Every write command is appended to the AOF file as a RESP array immediately
after execution. On startup, credis replays the file line by line, re-executing
each command to rebuild the in-memory state.

```
*3\r\n$3\r\nSET\r\n$4\r\nname\r\n$7\r\nemirhan\r\n
*3\r\n$6\r\nEXPIRE\r\n$4\r\nname\r\n$3\r\n300\r\n
```

## Building

Requires GCC (or MinGW on Windows) and make.

```sh
# Linux / macOS
make

# Windows (MinGW)
gcc -Wall -O2 -Iinclude -o credis.exe \
    src/core/main.c src/core/server.c src/core/config.c src/core/thread_pool.c \
    src/net/io.c src/store/db.c src/store/rlist.c src/store/rhash.c src/store/rset.c \
    src/resp/writer.c src/resp/parser.c src/persist/aof.c \
    src/commands/dispatcher.c src/commands/generic.c src/commands/string_cmds.c \
    src/commands/list_cmds.c src/commands/hash_cmds.c src/commands/set_cmds.c \
    -lws2_32
```

## Configuration

Settings are space-separated (not `key = value`):

```
port         6379
bind         0.0.0.0
maxclients   128
timeout      0       # 0 = no timeout
hz           10      # active expiry frequency
loglevel     notice  # debug / verbose / notice / warning

appendonly     yes
appendfilename credis.aof
```

Pass the config file as the first argument:

```sh
./credis credis.conf
```

## Usage

```sh
# Start the server
./credis credis.conf

# Connect with the official Redis CLI
redis-cli -p 6379

127.0.0.1:6379> SET name emirhan
OK
127.0.0.1:6379> GET name
"emirhan"
127.0.0.1:6379> EXPIRE name 60
(integer) 1
127.0.0.1:6379> TTL name
(integer) 58

127.0.0.1:6379> LPUSH queue task1 task2 task3
(integer) 3
127.0.0.1:6379> LRANGE queue 0 -1
1) "task3"
2) "task2"
3) "task1"

127.0.0.1:6379> HSET user:1 name emirhan age 25 city istanbul
(integer) 3
127.0.0.1:6379> HGETALL user:1
1) "name"
2) "emirhan"
3) "age"
4) "25"
5) "city"
6) "istanbul"

127.0.0.1:6379> SADD tags c systems redis
(integer) 3
127.0.0.1:6379> SMEMBERS tags
1) "redis"
2) "c"
3) "systems"
```

Any language's Redis client works out of the box:

```python
import redis
r = redis.Redis(host='localhost', port=6379)
r.set('key', 'value')
print(r.get('key'))
```

## Limitations

This is a learning project. Known limitations compared to production Redis:

- Single database (no `SELECT`)
- No pub/sub, transactions (`MULTI`/`EXEC`), or scripting (`EVAL`)
- No replication or clustering
- AOF is always fsync'd per command (no batching)
- No RDB snapshot support
- String values are null-terminated (no binary safety)
- No `SCAN` cursor — `KEYS` blocks while scanning
- Passwords stored in plain text (no `AUTH` support)
