# MyOwnRedis: A Simplified Redis Clone

This project implements a simplified version of **Redis** as an educational exercise, inspired by the ["Build Your Own Redis"](https://build-your-own.org/redis/) guide. The goal is to deepen the understanding of key components like hash tables, AVL trees, and client-server communication using sockets.

---

## Features

- **Client-Server Architecture**: Communication using sockets with a basic protocol for sending and receiving requests.
- **Core Commands**:
  - `SET key value` – Store a key-value pair.
  - `GET key` – Retrieve a value associated with a key.
  - `DEL key` – Delete a key-value pair.
  - `KEYS` – Retrieve all stored keys.
- **HashMap Implementation**: Efficient key-value storage using a progressive resizing hash table.
- **AVL Tree**: Support for ordered operations and balanced data structure management.

---

## Requirements

Ensure the following tools and libraries are installed:

- **C++ Compiler**: `g++` or `clang++`
- **System Libraries**: `arpa/inet.h`, `sys/socket.h` (for UNIX/Linux environments)
- **Development Environment**: A Linux or MacOS terminal (tested on Linux)

---

## File Structure

```bash
.
├── README.md # Documentation
├── client.cpp # Client implementation
├── server.cpp # Server implementation
├── libraries
│   ├── AVL.cpp # AVL Tree source
│   ├── AVL.h # AVL Tree header
│   ├── AVLTest.cpp # AVL Tree tests
│   ├── Common.h # Common macros and helpers
│   ├── HashTable.cpp # Hash table source
│   ├── HashTable.h # Hash table header
│   ├── HelperLibrary.cpp # Helper functions (I/O, errors)
│   ├── HelperLibrary.h # Helper header
│   └── ZSet.h # ZSet stub (for future use)
├── dump.rdb # Example dump file for persistence
├── myOwnRedis # Executable server binary
└── client # Executable client binary
```

---

## Installation

### 1. Build the Project

```bash
g++ -std=c++11 -o server server.cpp libraries/HashTable.cpp libraries/AVL.cpp libraries/HelperLibrary.cpp
g++ -std=c++11 -o client client.cpp libraries/HelperLibrary.cpp
```

### 2. Run the Server

Start the server on port 1234:

```bash
./server
```

### 3. Run the Client

Use the client to connect and interact with the server:

```bash
./client SET mykey "HelloWorld"
./client GET mykey
./client DEL mykey
./client KEYS
```

## Example Usage

### 1. Store a Key-Value Pair:

```bash
./client SET foo "bar"

Connected to the server!
(Nil)
```

### 2. Retrieve a Value:

```bash
./client GET foo

Connected to the server!
(str) bar
```

### 3. Delete a Key:

```bash
./client DEL foo

Connected to the server!
(int) 1
```

### 4. Retrieve All Keys:

```bash
./client KEYS
(arr) len = 1
foo
(arr) end
```

## Testing

The AVL Tree implementation is tested in AVLTest.cpp. To run tests:

```bash
g++ -std=c++11 -o AVLTest libraries/AVLTest.cpp libraries/AVL.cpp
./AVLTest
```

## Acknowledgments

- **[Build Your Own Redis](https://build-your-own.org/redis/)** – The inspiration and guidance for this project.
- **Redis** – The original in-memory key-value store that inspired this implementation.
- **C++ Standard Library** – Leveraging tools like sockets, hash tables, and balanced trees to understand core systems programming concepts.
- Special thanks to resources and communities that provided insights into data structures, client-server architectures, and best practices in C++ development.

## References

1. Build Your Own Redis: [https://build-your-own.org/redis/](https://build-your-own.org/redis/)
2. Redis Official Documentation: [https://redis.io/documentation](https://redis.io/documentation)
3. C++ Reference for `<cstdint>` and `<cstddef>`: [https://en.cppreference.com](https://en.cppreference.com)
4. Socket Programming Basics: [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
