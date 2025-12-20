# Ramora

Ramora is a high-performance, single-threaded in-memory caching engine written in C.

It is designed to be fast, predictable, and resource-efficient, with explicit control over memory, I/O, and event handling.

Ramora supports basic key-value operations, pipelined requests, TTL-based expiration, and connection lifecycle management — all built from scratch without external dependencies.

## Features

### High-performance event loop

- epoll-based I/O
- Fully non-blocking socket handling
- Explicit read/write state tracking

### In-memory key-value store

- supports GET, SET, and DEL operation for managing data
- Predictable memory usage

### TTL & Expiration

- Supports per-key TTL (seconds & milliseconds)
- Active expiration using a min-heap
- Immediate key removal upon expiry (not lazy)

### Connection management

- Idle connection tracking using doubly-linked lists
- Connection object pooling (bounded) (custom built reusing mechanism)
- Automatic cleanup of inactive connections

### Pipelining support

- High throughput under batch workloads
- Benchmarked up to millions of ops/sec on a single core

### Memory safety

- Zero memory leaks (verified via Valgrind)
- Explicit buffer management
- Bounded object pools to cap memory usage

## Architecture Overview

Ramora is intentionally built with explicit systems-level control:

- Event loop: epoll
- Networking: TCP, non-blocking sockets
- Concurrency model: single-threaded, single-core

### Data structures:

- Hash table for key storage (Uses progressive rehashing to cap worst case latency)
- Min-heap for TTL management
- Doubly-linked lists for idle connections and for lazyfreeing connection objects.

### Buffers:

- Custom dynamic read/write buffers
- Partial read/write handling
- No assumptions about TCP packet boundaries

## Project Structure

```
Ramora/
├── src/
│   ├── server.c
│   ├── buf.c
│   ├── cmd_proc.c
│   ├── config.c
│   ├── conn.c
│   ├── g_data.c
│   ├── heap.c
│   ├── hmap.c
│   ├── logging_helper.c
│   ├── oper.c
│   ├── response.c
│   ├── timer.c
│   └── vtr.c
├── include/
│   ├── buf.h
│   ├── cmd_proc.h
│   ├── config.h
│   ├── conn.h
│   ├── dlist.h
│   ├── g_data.h
│   ├── heap.h
│   ├── hmap.h
│   ├── logging_helper.h
│   ├── oper.h
│   ├── response.h
│   ├── timer.h
│   └── vtr.h
├── client/
│   └── client.c
├── conf/
│   └── ramora.conf
├── tests/
│   ├── testing_report.txt
│   └── test.c
├── Makefile
├── LICENSE
├── README.md
```

## Performance

Ramora is benchmarked using a custom client with configurable pipeline width, clients, number of operations, and predecided universal set of keys.

The testing report of Ramora with Redis is present in `tests/testing_report.txt`

Report have showed that Ramora is `12%` faster then Redis.

## Configuration

Ramora is configured via a server-side config file, allowing control over:

- Bind address
- Port
- Logfile's path
- Maximum load factor of hashmap
- Migrating load of hashmap when rehashing
- Initial hashmap capacity
- Initial heap capacity
- Initial buffer capacity
- Maximum buffer capacity
- Maximum allowed payload size
- Maximum pooled connection object's count
- Idle timeout for connections
- Read chunk size
- Maximum events that can be processed by one epoll cycle

To use a config file, see `conf/ramora.conf` for example.

To use a custom file, see the usage section below.

## Building

### To build binaries

run the below command in the root directory of this project.
```
make
```

This will create three binaries:

- ramora-server
- ramora-client
- ramora-test

### To clean up

run the below command to clean up all of the build files.
```
make clean
```

This will remove all the three binaries along with the build directory

## Usage

### Server

To use the server, just run the `ramora-server` binary.
```
./ramora-server
```

This will start the server.

If you want to run the server with custom config file, run the server with path of config file as argument.
```
./ramora-server path/to/config/ramora.conf
```

### Client

To use the client, just run the `ramora-client` binary (If you don't want error then make sure the server is already running).

```
./ramora-client
```

This will connect you to `127.0.0.1:5000`.

If you want to connect to some other host or port, use:
```
./ramora-client -h <ip_of_host> -p <port>
```

### Test

To use the test file, just run `ramora-test` binary.
```
./ramora-test
```

For help and option, run the binary with `-h` flag:

```
./ramora-test -h
```

## Author

### Dipanshu Tiwari
