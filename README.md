# 🧵 Scalable Thread Management Library

<div align="center">

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![pthreads](https://img.shields.io/badge/POSIX-Threads-4EAA25?style=for-the-badge)

**A high-performance, user-level thread management library in C** using POSIX threads (`pthreads`). Designed for Linux systems with efficient task scheduling via a fixed-size worker thread pool.

[Quick Start](#-quick-start) • [Features](#-features) • [Usage](#-api-usage) • [Tests](#-running-tests) • [Architecture](#-architecture)

</div>

---

## 📑 Table of Contents

- [✨ Features](#-features)
- [🏗️ Platform Requirements](#-platform-requirements)
- [🚀 Quick Start](#-quick-start)
  - [📦 Using Make](#-using-make-recommended)
  - [🖥️ Using CodeBlocks](#-using-codeblocks)
  - [⚙️ Manual Compilation](#-manual-compilation)
- [💻 API Usage](#-api-usage)
- [📂 Project Structure](#-project-structure)
- [🧪 Running Tests](#-running-tests)
- [🎯 Architecture](#-architecture)
- [⚡ Performance](#-performance)
- [💡 Design Decisions](#-design-decisions)
- [🔮 Future Enhancements](#-future-enhancements)
- [📜 License](#-license)

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| **Thread Pool** | Fixed number of reusable worker threads |
| **Thread-Safe Queue** | FIFO task queue with mutex + condition variable |
| **Task Submission** | Submit any function with arguments to the pool |
| **Graceful Shutdown** | Finishes pending tasks before stopping workers |
| **Statistics** | Track tasks submitted, completed, rejected, queue peaks |
| **Atomic Operations** | Lock-free stats using C11 `stdatomic.h` |
| **Configurable** | Customizable worker count and queue capacity |

## 🏗️ Platform Requirements

| Requirement | Details |
|-------------|---------|
| **OS** | Linux (Ubuntu recommended) |
| **Compiler** | GCC with C11 support |
| **Libraries** | `pthreads`, `librt` |
| **IDE** | CodeBlocks (project file included) |

## 🚀 Quick Start

### 📦 Using Make (Recommended)

```bash
# Build everything
cd thread-library
make

# Run all tests
make test

# Run demo
make demo

# Debug build
make debug

# Clean build artifacts
make clean
```

### 🖥️ Using CodeBlocks

1. Open `thread-pool.cbp` in CodeBlocks
2. Select target: **Debug** or **Release**
3. Build and run

### ⚙️ Manual Compilation

```bash
# Compile library sources
gcc -c -std=c11 -O2 -Iinclude src/core/*.c src/utils/*.c

# Link with your program
gcc -o myapp myapp.c task.o safe_queue.o thread_pool.o logger.o -lpthread -lrt
```

## 💻 API Usage

```c
#include "core/thread_pool.h"

// Your task function
void my_task(void *arg) {
    int value = *(int*)arg;
    printf("Processing: %d\n", value);
}

int main() {
    // Create pool with 4 workers
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(4);
    thread_pool_init(&pool, config);

    // Submit tasks
    int data = 42;
    thread_pool_submit(&pool, my_task, &data);

    // Wait for all tasks to complete
    thread_pool_wait_all(&pool);

    // View statistics
    thread_pool_print_stats(&pool);

    // Shutdown gracefully
    thread_pool_destroy(&pool);
    return 0;
}
```

## 📂 Project Structure

```
thread-library/
|
|-- include/
|   |-- core/
|   |   |-- task.h          # Task encapsulation
|   |   |-- safe_queue.h    # Thread-safe FIFO queue
|   |   |-- thread_pool.h   # Main thread pool API
|   |
|   |-- utils/
|       |-- logger.h        # Optional logging utility
|
|-- src/
|   |-- core/
|   |   |-- task.c          # Task implementation
|   |   |-- safe_queue.c    # Queue implementation
|   |   |-- thread_pool.c   # Pool and worker management
|   |
|   |-- utils/
|       |-- logger.c        # Logger implementation
|
|-- tests/
|   |-- test_basic.c        # Basic functionality tests
|   |-- test_stress.c       # Performance and stress tests
|   |-- test_shutdown.c     # Shutdown behavior tests
|
|-- examples/
|   |-- demo.c              # Practical usage examples
|
|-- build/                  # Compiled binaries (generated)
|-- Makefile               # Build configuration
|-- thread-pool.cbp         # CodeBlocks project file
|-- README.md              # This file
```

## 🧪 Running Tests

### ✅ Basic Tests
```bash
make test_basic
```
Tests pool creation, task execution, arguments, queue size, and concurrent execution.

### 💪 Stress Tests
```bash
make test_stress
```
Tests with 1K, 10K, and 100K tasks. Measures throughput (tasks/sec) and latency.

### 🔌 Shutdown Tests
```bash
make test_shutdown
```
Tests graceful shutdown, task rejection after shutdown, double shutdown safety, and empty pool handling.

## 🎯 Architecture

### 🔧 Components

```
+-----------+    +------------------+    +-----------+
|   User    |--->|  ThreadPool API  |--->| SafeQueue |
|  Code     |    |  (thread_pool.h) |    |(mutex+CV) |
+-----------+    +------------------+    +-----+-----+
                                                |
                      +-------------------------+
                      |
                +-----v-----+     +-----v-----+     +-----v-----+
                |  Worker 0 |     |  Worker 1 |     |  Worker N |
                | (pthread) |     | (pthread) |     | (pthread) |
                +-----+-----+     +-----+-----+     +-----+-----+
                      |                 |                 |
                      +---- loop: pop task & execute -----+
```

### 🔄 Worker Thread Loop

```
while (pool running):
    wait for task from queue (blocking)
    if shutdown signal:
        exit loop
    execute task
    update statistics
```

### 🛑 Graceful Shutdown Sequence

1. Set state to `SHUTTING_DOWN` - rejects new submissions
2. Signal the queue to wake all waiting workers
3. Each worker finishes its current task
4. Workers drain remaining tasks from the queue
5. All worker threads join (terminate cleanly)
6. State becomes `SHUTDOWN`

## ⚡ Performance

Typical results on a modern quad-core Linux system:

| Test | Tasks | Time | Throughput |
|------|-------|------|------------|
| Fast tasks | 1,000 | ~5 ms | ~200,000/sec |
| Fast tasks | 10,000 | ~30 ms | ~330,000/sec |
| Computation tasks | 10,000 | ~200 ms | ~50,000/sec |

*Actual results depend on CPU cores, clock speed, and system load.*

## 💡 Design Decisions

| Decision | Rationale |
|----------|-----------|
| **C instead of C++** | Simpler, more portable, direct pthread control |
| **Fixed thread count** | Predictable resource usage, avoids churn |
| **Mutex + CV queue** | Simple, correct, efficient for most workloads |
| **Linked-list queue** | Dynamic size, simple implementation |
| `stdatomic.h` stats | Lock-free statistics, no contention |
| `void*` task args | Generic API, any data type |

## 🔮 Future Enhancements

- [ ] **Priority Queue** - Replace FIFO with priority-based scheduling
- [ ] **Dynamic Pool** - Auto-scale worker count based on load
- [ ] **Work Stealing** - Idle workers steal from busy workers' queues
- [ ] **Lock-Free Queue** - Lock-free data structure for higher performance
- [ ] **Task Dependencies** - Support task chains and directed acyclic graphs
- [ ] **Timed Submit** - Submit with timeout and deadline scheduling

## 📜 License

This project is provided for educational purposes. Free to use and modify.
