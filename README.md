# Mini Redis in C

A simplified implementation of Redis in C, built as the capstone project after completing six portfolio data structures (circular buffer, linked list, dynamic array, hash map, binary search tree, thread-safe queue) and a multi threaded TCP echo server. This project ties those pieces together into a working client-server system.

## Scope

This is a "mini" Redis in the literal sense: it implements a single native data type (string, backed by the hash map from the [systems-programming-journey](https://github.com/relense/systems-programming-journey)) rather than the full range Redis supports (lists, sets, sorted sets, hashes, streams). The goal is to understand the full path from raw bytes on a socket to a command being executed and a response being sent back, not to replicate every feature.

## Planned commands

- `PING`
- `SET key value`
- `GET key`
- `DEL key`

## Protocol

Implements a subset of [RESP (REdis Serialization Protocol)](https://redis.io/docs/latest/develop/reference/protocol-spec/): simple strings, simple errors, bulk strings, and arrays. Clients send commands as an array of bulk strings; the server parses the command, executes it against the in-memory store, and encodes the result back into RESP.

## Architecture

- TCP server (sockets, one thread per client connection)
- Byte buffer (dynamically growing buffer that accumulates incoming bytes across multiple recv() calls until a complete message is available)
- RESP parser (decodes accumulated bytes into commands and arguments)
- RESP encoder (formats command results back into valid RESP replies)
- In-memory storage (thread-safe hash map)

## Status

In progress. Currently building on the echo server foundation from `systems-programming-journey`.

## Compile

```bash
gcc -Wall -Wextra -Werror -std=c17 -fsanitize=address -g -o mini-redis.out src/*.c -lpthread
```

- **gcc** - The compiler. The program that transforms C into an executable.
- **-Wall** - "Warnings ALL"
  - Shows ALL warnings (suspicious code, unused variables, etc)
  - Example: if you have a variable you never use, it warns you
- **-Wextra** - "Warnings EXTRA"
  - More warnings beyond -Wall
  - Even more strict
  - Example: strange comparisons, non-intuitive things
- **-Werror** - "Warnings are ERRORS"
  - If there's a warning, **it won't compile**
  - Forces you to write flawless code from the start
  - Without it, it would compile even with warnings (bad)
- **-std=c17** - "Standard C17"
  - Specifies which version of C to use
  - C17 is the most recent (released in 2018)
  - Guarantees modern features and security
- **-fsanitize=address** - "Address Sanitizer"
  - Detects memory errors **during execution**
  - Like Valgrind, but faster
  - Warns if:
    - You free memory and continue using it (use-after-free)
    - You write outside the bounds of an array
    - Memory leaks (you allocated but never freed)
- **-g** - "Debug symbols"
  - Includes debug information in the executable
  - If the program crashes, you see exactly which line
  - Without it you'd see strange numbers instead of line numbers
- **-lpthread** - "Link pthread"
  - Links the POSIX threads library
  - Needed whenever the program uses `pthread_create`, `pthread_mutex_t`, `pthread_cond_t`, or any other pthread function
  - Without it, the linker won't find the implementations of these functions and compilation fails

  ## Run

```bash
   ./mini-redis
```
