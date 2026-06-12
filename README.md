# Treebugger

Treebugger is a Linux-x86_64 userspace debugger written in C++ using `ptrace`. The project is being built as a learning exercise to better understand debugger internals, process control, Linux system calls, and checkpointing mechanisms.

The long-term goal is to explore debugger checkpointing through process forking and process tree management.

---

## Features

### Execution Control

* Step execution (`step`)
* Continue execution (`continue`)

### Register Operations

* Read register values (`register_read`)
* Modify register values (`register_write`)

### Disassembly

* Disassemble instructions around the current instruction pointer (`disasm`)
* Powered by the Capstone disassembly engine

### Breakpoints

* Set software breakpoints (`break`, `b`)
* Delete breakpoints (`delete`, `d`)

### Process Management

* Attach to and control Linux processes using `ptrace`
* Read and modify process state

### Experimental Checkpointing

* Create checkpoints using process forking (`checkpoint`)
* Checkpoints are represented internally as a process tree
* Active area of development

---

## Architecture

### Debugger Core

Treebugger communicates with the target process through Linux `ptrace`.

The debugger currently supports:

* Register inspection
* Register modification
* Single stepping
* Continuing execution
* Breakpoint management
* Instruction disassembly

### Disassembly Engine

Capstone is used to decode machine instructions and display human-readable assembly around the current instruction pointer.

### Command System

Commands are mapped to handler functions through an internal command dispatcher.

Current commands:

* `step`
* `continue`
* `register_read`
* `register_write`
* `disasm`
* `break`
* `delete`
* `checkpoint`
* `quit`

---

## Checkpointing Research

One of the primary goals of Treebugger is exploring debugger checkpoints without requiring full memory snapshots.

Current approach:

1. Force the debuggee to execute a `fork()` syscall.
2. Preserve one process as a checkpoint.
3. Continue execution in the other process.
4. Represent checkpoints as nodes in a process tree.

This design is inspired by Linux Copy-On-Write behavior, where memory pages are shared until modified.

The checkpoint subsystem is still under active development and several process-control challenges remain under investigation.

---

## Technologies Used

* C++
* Linux `ptrace`
* Capstone Disassembly Framework
* Meson
* Ninja

---

## Current Learning Goals

This project is primarily focused on understanding:

* Debugger implementation
* Linux process management
* Register state manipulation
* Software breakpoints
* Instruction decoding
* Process forking
* Copy-On-Write memory behavior
* Checkpoint and rollback architectures
* Low-level systems programming

---

## Status

Work in progress.

Current development is focused on improving the checkpointing architecture and process tree management system.
