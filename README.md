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
- RESP parser (decodes incoming byte streams into commands and arguments)
- RESP encoder (formats command results back into valid RESP replies)
- In-memory storage (thread-safe hash map)

## Status

In progress. Currently building on the echo server foundation from `systems-programming-journey`.

## Compile

_TBD, will be added once the build is in place._
