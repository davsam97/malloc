<h1 align="center">🧠 malloc</h1>

<p align="center">
  <img src="https://img.shields.io/badge/language-C-A8B9CC?logo=c&logoColor=white" alt="C">
  <img src="https://img.shields.io/badge/syscalls-mmap%20%2F%20munmap-blue" alt="mmap / munmap">
  <img src="https://img.shields.io/badge/thread--safe-pthread-success" alt="thread-safe">
  <img src="https://img.shields.io/badge/LD__PRELOAD-ready-brightgreen" alt="LD_PRELOAD ready">
  <img src="https://img.shields.io/badge/school-42-000000?logo=42&logoColor=white" alt="42">
</p>

<p align="center">
A reimplementation of the C memory allocator as a shared library, built only on <code>mmap</code> and <code>munmap</code>.<br>
Drop it in with <code>LD_PRELOAD</code> and <code>ls</code>, <code>git</code>, <code>vim</code> or <code>python3</code> run on it.
</p>

*This project has been created as part of the 42 curriculum by dasamuel.*

Built **solo** (about 49 hours, 9450 XP).

**Score: 125/100** ✅

<p align="center">
  <img src="assets/score.png" alt="42 project result: malloc, success, 125/100" width="700">
</p>

## 📖 Table of contents

- [About](#-about)
- [Features](#-features)
- [Build](#-build)
- [Usage](#-usage)
- [Debug variables](#-debug-variables)
- [Project structure](#-project-structure)
- [How it works](#-how-it-works)
- [Skills demonstrated](#-skills-demonstrated)
- [Resources](#-resources)
- [Author](#-author)

## 🎯 About

`malloc` is not a system call: it is a library function that asks the kernel for pages and
then cuts them into the small pieces programs actually request. This project rebuilds that
layer from scratch as `libft_malloc.so`, which exports `malloc`, `free`, `realloc` and
`show_alloc_mem`.

The only way to get memory from the system is `mmap`, and the only way to give it back is
`munmap`. Everything in between (zones, block headers, free lists, splitting, merging,
pointer validation) is handled by the library.

Because it exports the standard symbols, the library can **replace the system allocator in
any existing program** through `LD_PRELOAD`, without recompiling it.

The goal: understand what really happens behind `malloc`, and design an allocator that wastes
few pages, answers fast and survives invalid pointers.

## ✨ Features

**Mandatory part**

- `malloc`, `free`, `realloc` built only on `mmap` / `munmap`
- **TINY / SMALL / LARGE** strategy: pre-allocated zones for small requests, one mapping per large one
- 16-byte aligned memory
- `show_alloc_mem()` prints every allocated block, zone by zone, in address order
- Invalid pointers given to `free` / `realloc` are ignored instead of corrupting memory
- Only two globals, as the subject allows

**Bonus**

- **Thread safety**: a `pthread` mutex protects every entry point
- **Defragmentation**: a freed block is merged with its free neighbours
- **`show_alloc_mem_ex()`**: lists every block, free ones included
- **Hexdump**: `show_alloc_mem_ex()` dumps the content of each allocated block
- **Allocation history**: the last 64 operations are shown by `show_alloc_mem_ex()`
- **Debug environment variables**: scribble, verbose logging and pointer checks (see [below](#-debug-variables))
- **libc compatibility**: `calloc`, `reallocarray`, `malloc_usable_size`, `memalign`,
  `posix_memalign`, `aligned_alloc` and `valloc` are replaced too, so no glibc pointer can
  ever reach our `free`

## 🛠 Build

Requirements: a C compiler (`cc` / `gcc` / `clang`), `make`, Linux or macOS.

```bash
git clone https://github.com/davsam97/malloc.git
cd malloc

make        # builds libft_malloc_$HOSTTYPE.so and the libft_malloc.so symlink
make test   # runs the test suite
make clean  # removes object files and test binaries
make fclean # also removes the libraries
make re     # full rebuild
```

The library compiles with `cc -Wall -Wextra -Werror -fPIC`. A linker version script
([`exports.map`](exports.map)) exports only the public functions, and the binary is stripped. `HOSTTYPE` defaults to
`$(uname -m)_$(uname -s)` when it is not set.

## 🚀 Usage

Run any existing program on top of it:

```bash
LD_PRELOAD=./libft_malloc.so ls -la
LD_PRELOAD=./libft_malloc.so python3 -c "print('hello')"
```

Or use the provided wrapper:

```bash
sh tests/run.sh vim
```

Or link it into your own program:

```bash
cc -I includes main.c -L. -lft_malloc
LD_LIBRARY_PATH=. ./a.out
```

Example output of `show_alloc_mem()`:

```
SMALL : 0x7F6F92376000
0x7F6F92376040 - 0x7F6F92376360 : 800 bytes
TINY : 0x7F6F925A0000
0x7F6F925A0040 - 0x7F6F925A006A : 42 bytes
Total : 842 bytes
```

### Tests

`make test` follows the evaluation sheet step by step: `test0` to `test6` (including the
page-reclaim count, measured with ASLR disabled when possible), then bonus tests for
threads, `fork`, defragmentation, zones, edge cases and the debug variables.

## 🐞 Debug variables

| Variable               | Effect                                                         |
|------------------------|----------------------------------------------------------------|
| `FT_MALLOC_SCRIBBLE=1` | Fills allocated memory with `0xAA` and freed memory with `0x55`. |
| `FT_MALLOC_VERBOSE=1`  | Logs every operation on stderr.                                |
| `FT_MALLOC_CHECK=1`    | Reports invalid `free` and `realloc` calls.                    |
| `FT_MALLOC_CHECK=2`    | Same, and aborts the program.                                  |

```bash
FT_MALLOC_VERBOSE=1 LD_PRELOAD=./libft_malloc.so ls
```

## 🗂 Project structure

```
malloc/
├── Makefile
├── exports.map         # symbols exported by the library, everything else stays hidden
├── malloc.h -> includes/malloc.h
├── includes/
│   └── malloc.h        # public API
├── srcs/
│   ├── api/            # malloc, free, realloc, calloc, memalign & co
│   ├── core/           # globals, heap state, pointer lookup
│   ├── zone/           # zone creation / release, sorted zone table
│   ├── block/          # block headers, splitting, merging, free lists
│   ├── show/           # show_alloc_mem, show_alloc_mem_ex, hexdump
│   ├── debug/          # FT_MALLOC_* environment variables, history
│   └── utils/          # libc-free helpers (ft_memcpy, ft_memset, ft_put*_fd)
├── tests/              # evaluation tests, bonus tests, correction.sh
└── assets/             # images used by this README
```

## 🧠 How it works

### Zones

| Type  | Size         | Storage                                           |
|-------|--------------|---------------------------------------------------|
| TINY  | ≤ 128 bytes  | 16 KB zones (4 pages), at least 100 blocks each   |
| SMALL | ≤ 1024 bytes | 104 KB zones (26 pages), at least 100 blocks each |
| LARGE | > 1024 bytes | one `mmap` per allocation                         |

Small requests are the most common. Grouping them into pre-allocated zones avoids a system
call and a wasted page for every `malloc`. Large blocks get their own mapping, which is given
back to the system as soon as they are freed.

### Blocks and free lists

Inside a zone, memory is a linked list of blocks. Each block starts with a **32-byte header**
(its size, whether it is free, and the next block), and its data is aligned on 16 bytes.

Free blocks are also sorted by size into **segregated lists**, one per multiple of 16 bytes.
A fitting block is found right away, without walking the zone. A block that is bigger than
needed is split in two.

```
 zone header │ hdr │ data (used) │ hdr │ free ──────┐ │ hdr │ data (used) │ ...
                                          ▲         │
                    free list [size/16] ──┘         └──▶ next free block of the same size
```

### Freeing

A freed block is merged with the free blocks around it, so free memory stays in large pieces.
Empty zones are given back to the system, **except one per type**, which is kept to avoid
calling `mmap` and `munmap` over and over.

### Finding a pointer

All zones are kept in an **array sorted by address**. `free` and `realloc` find the zone of a
pointer with a binary search, then check that it really is an allocated block. Invalid
pointers are ignored instead of corrupting memory. `show_alloc_mem` uses the same array to
print the zones in address order.

`realloc` resizes the block in place when it can. Otherwise it allocates a new block and
copies the data.

### Globals

There are only two globals, as the subject allows: `g_heap` holds the allocator state and
`g_mutex` provides thread safety.

## 💡 Skills demonstrated

- Virtual memory and paging: `mmap`, `munmap`, `getpagesize`, page reclaims
- Memory allocation algorithms: zones, segregated free lists, splitting, coalescing
- Low-level C: pointer arithmetic, alignment, in-band metadata
- Building and exporting a shared library, symbol interposition with `LD_PRELOAD`
- Thread safety with `pthread` mutexes, and correct behaviour across `fork`
- Defensive programming: validating untrusted pointers without crashing
- Writing a test suite that mirrors an evaluation sheet

## 📚 Resources

- Linux man pages: `mmap(2)`, `munmap(2)`, `malloc(3)`, `pthread_mutex_lock(3)`, `ld.so(8)`
- **glibc wiki** — *Malloc Internals*: <https://sourceware.org/glibc/wiki/MallocInternals>

### Use of AI

AI assistance was used as a support tool, not as a replacement for my own implementation.
It helped with reviewing parts of the code. All design decisions and the allocator itself 
were written and validated by me.

## 👤 Author

- **David Samuel** ([davsam97](https://github.com/davsam97)) — 42 login: `dasamuel`
